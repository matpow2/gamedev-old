#include "font.h"

// FTLibrary

const FTLibrary&  FTLibrary::Instance()
{
    static FTLibrary ftlib;
    return ftlib;
}


FTLibrary::~FTLibrary()
{
    FTCleanup::Instance()->DestroyAll();

    if(library != 0)
    {
        FT_Done_FreeType(*library);

        delete library;
        library= 0;
    }
}


FTLibrary::FTLibrary()
:   library(0),
    err(0)
{
    Initialise();
}


bool FTLibrary::Initialise()
{
    if(library != 0)
        return true;

    library = new FT_Library;

    err = FT_Init_FreeType(library);
    if(err)
    {
        delete library;
        library = 0;
        return false;
    }

    FTCleanup::Instance();

    return true;
}

// FTSize

FTSize::FTSize()
:   ftFace(0),
    ftSize(0),
    size(0),
    xResolution(0),
    yResolution(0),
    err(0)
{}


FTSize::~FTSize()
{}


bool FTSize::CharSize(FT_Face* face, unsigned int pointSize, unsigned int xRes, unsigned int yRes)
{
    if(size != pointSize || xResolution != xRes || yResolution != yRes)
    {
        err = FT_Set_Char_Size(*face, 0L, pointSize * 64, xResolution, yResolution);

        if(!err)
        {
            ftFace = face;
            size = pointSize;
            xResolution = xRes;
            yResolution = yRes;
            ftSize = (*ftFace)->size;
        }
    }

    return !err;
}


unsigned int FTSize::CharSize() const
{
    return size;
}


float FTSize::Ascender() const
{
    return ftSize == 0 ? 0.0f : static_cast<float>(ftSize->metrics.ascender) / 64.0f;
}


float FTSize::Descender() const
{
    return ftSize == 0 ? 0.0f : static_cast<float>(ftSize->metrics.descender) / 64.0f;
}


float FTSize::Height() const
{
    if(0 == ftSize)
    {
        return 0.0f;
    }

    if(FT_IS_SCALABLE((*ftFace)))
    {
        return ((*ftFace)->bbox.yMax - (*ftFace)->bbox.yMin) * ((float)ftSize->metrics.y_ppem / (float)(*ftFace)->units_per_EM);
    }
    else
    {
        return static_cast<float>(ftSize->metrics.height) / 64.0f;
    }
}


float FTSize::Width() const
{
    if(0 == ftSize)
    {
        return 0.0f;
    }

    if(FT_IS_SCALABLE((*ftFace)))
    {
        return ((*ftFace)->bbox.xMax - (*ftFace)->bbox.xMin) * (static_cast<float>(ftSize->metrics.x_ppem) / static_cast<float>((*ftFace)->units_per_EM));
    }
    else
    {
        return static_cast<float>(ftSize->metrics.max_advance) / 64.0f;
    }
}


float FTSize::Underline() const
{
    return 0.0f;
}

// FTFace

FTFace::FTFace(const char* fontFilePath, bool precomputeKerning)
:   numGlyphs(0),
    fontEncodingList(0),
    kerningCache(0),
    err(0)
{
    const FT_Long DEFAULT_FACE_INDEX = 0;
    ftFace = new FT_Face;

    err = FT_New_Face(*FTLibrary::Instance().GetLibrary(), fontFilePath,
                      DEFAULT_FACE_INDEX, ftFace);
    if(err)
    {
        delete ftFace;
        ftFace = 0;
        return;
    }

    FTCleanup::Instance()->RegisterObject(&ftFace);

    numGlyphs = (*ftFace)->num_glyphs;
    hasKerningTable = (FT_HAS_KERNING((*ftFace)) != 0);

    if(hasKerningTable && precomputeKerning)
    {
        BuildKerningCache();
    }
}


FTFace::FTFace(const unsigned char *pBufferBytes, size_t bufferSizeInBytes,
               bool precomputeKerning)
:   numGlyphs(0),
    fontEncodingList(0),
    kerningCache(0),
    err(0)
{
    const FT_Long DEFAULT_FACE_INDEX = 0;
    ftFace = new FT_Face;

    err = FT_New_Memory_Face(*FTLibrary::Instance().GetLibrary(),
                             (FT_Byte const *)pBufferBytes, (FT_Long)bufferSizeInBytes,
                             DEFAULT_FACE_INDEX, ftFace);
    if(err)
    {
        delete ftFace;
        ftFace = 0;
        return;
    }

    FTCleanup::Instance()->RegisterObject(&ftFace);

    numGlyphs = (*ftFace)->num_glyphs;
    hasKerningTable = (FT_HAS_KERNING((*ftFace)) != 0);

    if(hasKerningTable && precomputeKerning)
    {
        BuildKerningCache();
    }
}


FTFace::~FTFace()
{
    delete[] kerningCache;

    if(ftFace)
    {
        FTCleanup::Instance()->UnregisterObject(&ftFace);

        FT_Done_Face(*ftFace);
        delete ftFace;
        ftFace = 0;
    }
}


bool FTFace::Attach(const char* fontFilePath)
{
    err = FT_Attach_File(*ftFace, fontFilePath);
    return !err;
}


bool FTFace::Attach(const unsigned char *pBufferBytes,
                    size_t bufferSizeInBytes)
{
    FT_Open_Args open;

    open.flags = FT_OPEN_MEMORY;
    open.memory_base = (FT_Byte const *)pBufferBytes;
    open.memory_size = (FT_Long)bufferSizeInBytes;

    err = FT_Attach_Stream(*ftFace, &open);
    return !err;
}


const FTSize& FTFace::Size(const unsigned int size, const unsigned int res)
{
    charSize.CharSize(ftFace, size, res, res);
    err = charSize.Error();

    return charSize;
}


unsigned int FTFace::CharMapCount() const
{
    return (*ftFace)->num_charmaps;
}


FT_Encoding* FTFace::CharMapList()
{
    if(0 == fontEncodingList)
    {
        fontEncodingList = new FT_Encoding[CharMapCount()];
        for(size_t i = 0; i < CharMapCount(); ++i)
        {
            fontEncodingList[i] = (*ftFace)->charmaps[i]->encoding;
        }
    }

    return fontEncodingList;
}


FTPoint FTFace::KernAdvance(unsigned int index1, unsigned int index2)
{
    FTGL_DOUBLE x, y;

    if(!hasKerningTable || !index1 || !index2)
    {
        return FTPoint(0.0, 0.0);
    }

    if(kerningCache && index1 < FTFace::MAX_PRECOMPUTED
        && index2 < FTFace::MAX_PRECOMPUTED)
    {
        x = kerningCache[2 * (index2 * FTFace::MAX_PRECOMPUTED + index1)];
        y = kerningCache[2 * (index2 * FTFace::MAX_PRECOMPUTED + index1) + 1];
        return FTPoint(x, y);
    }

    FT_Vector kernAdvance;
    kernAdvance.x = kernAdvance.y = 0;

    err = FT_Get_Kerning(*ftFace, index1, index2, ft_kerning_unfitted,
                         &kernAdvance);
    if(err)
    {
        return FTPoint(0.0f, 0.0f);
    }

    x = static_cast<float>(kernAdvance.x) / 64.0f;
    y = static_cast<float>(kernAdvance.y) / 64.0f;

    return FTPoint(x, y);
}


FT_GlyphSlot FTFace::Glyph(unsigned int index, FT_Int load_flags)
{
    err = FT_Load_Glyph(*ftFace, index, load_flags);
    if(err)
    {
        return NULL;
    }

    return (*ftFace)->glyph;
}


void FTFace::BuildKerningCache()
{
    FT_Vector kernAdvance;
    kernAdvance.x = 0;
    kernAdvance.y = 0;
    kerningCache = new FTGL_DOUBLE[FTFace::MAX_PRECOMPUTED
                                    * FTFace::MAX_PRECOMPUTED * 2];
    for(unsigned int j = 0; j < FTFace::MAX_PRECOMPUTED; j++)
    {
        for(unsigned int i = 0; i < FTFace::MAX_PRECOMPUTED; i++)
        {
            err = FT_Get_Kerning(*ftFace, i, j, ft_kerning_unfitted,
                                 &kernAdvance);
            if(err)
            {
                delete[] kerningCache;
                kerningCache = NULL;
                return;
            }

            kerningCache[2 * (j * FTFace::MAX_PRECOMPUTED + i)] =
                                static_cast<FTGL_DOUBLE>(kernAdvance.x) / 64.0;
            kerningCache[2 * (j * FTFace::MAX_PRECOMPUTED + i) + 1] =
                                static_cast<FTGL_DOUBLE>(kernAdvance.y) / 64.0;
        }
    }
}

FTFont::FTFont(char const *fontFilePath) :
    face(fontFilePath),
    load_flags(FT_LOAD_DEFAULT),
    glyphList(0)
{
    err = face.Error();
    if(err == 0)
    {
        glyphList = new FTGlyphContainer(&face);
    }
}


FTFont::~FTFont()
{
    if(glyphList)
    {
        delete glyphList;
    }
}


bool FTFont::FaceSize(const unsigned int size, const unsigned int res)
{
    if(glyphList != NULL)
    {
        delete glyphList;
        glyphList = NULL;
    }

    charSize = face.Size(size, res);
    err = face.Error();

    if(err != 0)
    {
        return false;
    }

    glyphList = new FTGlyphContainer(&face);
    return true;
}


unsigned int FTFont::FaceSize() const
{
    return charSize.CharSize();
}

void FTFont::GlyphLoadFlags(FT_Int flags)
{
    load_flags = flags;
}


bool FTFont::CharMap(FT_Encoding encoding)
{
    bool result = glyphList->CharMap(encoding);
    err = glyphList->Error();
    return result;
}


unsigned int FTFont::CharMapCount() const
{
    return face.CharMapCount();
}


FT_Encoding* FTFont::CharMapList()
{
    return face.CharMapList();
}

float FTFont::Ascender() const
{
    return charSize.Ascender();
}


float FTFont::Descender() const
{
    return charSize.Descender();
}


float FTFont::LineHeight() const
{
    return charSize.Height();
}


template <typename T>
inline FTBBox FTFont::BBoxI(const T* string, const int len,
                                FTPoint position, FTPoint spacing)
{
    FTBBox totalBBox;

    /* Only compute the bounds if string is non-empty. */
    if(string && ('\0' != string[0]))
    {
        // for multibyte - we can't rely on sizeof(T) == character
        FTUnicodeStringItr<T> ustr(string);
        unsigned int thisChar = *ustr++;
        unsigned int nextChar = *ustr;

        if(CheckGlyph(thisChar))
        {
            totalBBox = glyphList->BBox(thisChar);
            totalBBox += position;

            position += FTPoint(glyphList->Advance(thisChar, nextChar), 0.0);
        }

        /* Expand totalBox by each glyph in string */
        for(int i = 1; (len < 0 && *ustr) || (len >= 0 && i < len); i++)
        {
            thisChar = *ustr++;
            nextChar = *ustr;

            if(CheckGlyph(thisChar))
            {
                position += spacing;

                FTBBox tempBBox = glyphList->BBox(thisChar);
                tempBBox += position;
                totalBBox |= tempBBox;

                position += FTPoint(glyphList->Advance(thisChar, nextChar),
                                    0.0);
            }
        }
    }

    return totalBBox;
}


FTBBox FTFont::BBox(const char *string, const int len,
                        FTPoint position, FTPoint spacing)
{
    /* The chars need to be unsigned because they are cast to int later */
    return BBoxI((const unsigned char *)string, len, position, spacing);
}


FTBBox FTFont::BBox(const wchar_t *string, const int len,
                        FTPoint position, FTPoint spacing)
{
    return BBoxI(string, len, position, spacing);
}


template <typename T>
inline float FTFont::AdvanceI(const T* string, const int len,
                                  FTPoint spacing)
{
    float advance = 0.0f;
    FTUnicodeStringItr<T> ustr(string);

    for(int i = 0; (len < 0 && *ustr) || (len >= 0 && i < len); i++)
    {
        unsigned int thisChar = *ustr++;
        unsigned int nextChar = *ustr;

        if(CheckGlyph(thisChar))
        {
            advance += glyphList->Advance(thisChar, nextChar);
        }

        if(nextChar)
        {
            advance += spacing.Xf();
        }
    }

    return advance;
}


float FTFont::Advance(const char* string, const int len, FTPoint spacing)
{
    /* The chars need to be unsigned because they are cast to int later */
    const unsigned char *ustring = (const unsigned char *)string;
    return AdvanceI(ustring, len, spacing);
}


float FTFont::Advance(const wchar_t* string, const int len, FTPoint spacing)
{
    return AdvanceI(string, len, spacing);
}


template <typename T>
inline FTPoint FTFont::RenderI(const T* string, const int len,
                                   FTPoint position, FTPoint spacing,
                                   int renderMode)
{
    // for multibyte - we can't rely on sizeof(T) == character
    FTUnicodeStringItr<T> ustr(string);

    for(int i = 0; (len < 0 && *ustr) || (len >= 0 && i < len); i++)
    {
        unsigned int thisChar = *ustr++;
        unsigned int nextChar = *ustr;

        if(CheckGlyph(thisChar))
        {
            position += glyphList->Render(thisChar, nextChar,
                                          position, renderMode);
        }

        if(nextChar)
        {
            position += spacing;
        }
    }

    return position;
}


FTPoint FTFont::Render(const char * string, const int len,
                           FTPoint position, FTPoint spacing, int renderMode)
{
    return RenderI((const unsigned char *)string,
                   len, position, spacing, renderMode);
}


FTPoint FTFont::Render(const wchar_t * string, const int len,
                           FTPoint position, FTPoint spacing, int renderMode)
{
    return RenderI(string, len, position, spacing, renderMode);
}


bool FTFont::CheckGlyph(const unsigned int characterCode)
{
    if(glyphList->Glyph(characterCode))
    {
        return true;
    }

    unsigned int glyphIndex = glyphList->FontIndex(characterCode);
    FT_GlyphSlot ftSlot = face.Glyph(glyphIndex, load_flags);
    if(!ftSlot)
    {
        err = face.Error();
        return false;
    }

    FTGlyph* tempGlyph = MakeGlyph(ftSlot);
    if(!tempGlyph)
    {
        if(0 == err)
        {
            err = 0x13;
        }

        return false;
    }

    glyphList->Add(tempGlyph, characterCode);

    return true;
}

// FTGlyph


FTGlyph::FTGlyph(FT_GlyphSlot glyph) : err(0)
{
    if(glyph)
    {
        bBox = FTBBox(glyph);
        advance = FTPoint(glyph->advance.x / 64.0f,
                          glyph->advance.y / 64.0f);
    }
}


FTGlyph::~FTGlyph()
{}


float FTGlyph::Advance() const
{
    return advance.Xf();
}


const FTBBox& FTGlyph::BBox() const
{
    return bBox;
}


FT_Error FTGlyph::Error() const
{
    return err;
}

// FTTextureFont

static inline GLuint ClampSize(GLuint in, GLuint maxTextureSize)
{
    // Find next power of two
    --in;
    in |= in >> 16;
    in |= in >> 8;
    in |= in >> 4;
    in |= in >> 2;
    in |= in >> 1;
    ++in;

    // Clamp to max texture size
    return in < maxTextureSize ? in : maxTextureSize;
}

class FTTextureFont : public FTFont
{
public:
    GLsizei maximumGLTextureSize;
    GLsizei textureWidth;
    GLsizei textureHeight;
    FTVector<GLuint> textureIDList;
    int glyphHeight;
    int glyphWidth;
    unsigned int padding;
    unsigned int numGlyphs;
    unsigned int remGlyphs;
    int xOffset;
    int yOffset;
    bool stroke;

    FTTextureFont(const char* fontFilePath, bool stroke)
    :   FTFont(fontFilePath),
        maximumGLTextureSize(0),
        textureWidth(0),
        textureHeight(0),
        glyphHeight(0),
        glyphWidth(0),
        xOffset(0),
        yOffset(0),
        padding(3),
        stroke(stroke)
    {
        load_flags = FT_LOAD_NO_HINTING | FT_LOAD_NO_BITMAP;
        remGlyphs = numGlyphs = face.GlyphCount();
        if (stroke) {
            padding += 4;
        }
    }


    ~FTTextureFont()
    {
        if(textureIDList.size())
        {
            glDeleteTextures((GLsizei)textureIDList.size(),
                             (const GLuint*)&textureIDList[0]);
        }
    }


    FTGlyph* MakeGlyph(FT_GlyphSlot ftGlyph)
    {
        glyphHeight = static_cast<int>(charSize.Height() + 0.5f);
        glyphWidth = static_cast<int>(charSize.Width() + 0.5f);

        if(glyphHeight < 1) glyphHeight = 1;
        if(glyphWidth < 1) glyphWidth = 1;

        if(textureIDList.empty())
        {
            textureIDList.push_back(CreateTexture());
            xOffset = yOffset = padding;
        }

        if(xOffset > (textureWidth - glyphWidth))
        {
            xOffset = padding;
            yOffset += glyphHeight;

            if(yOffset > (textureHeight - glyphHeight))
            {
                textureIDList.push_back(CreateTexture());
                yOffset = padding;
            }
        }

        FTTextureGlyph* tempGlyph = new FTTextureGlyph(ftGlyph, textureIDList[textureIDList.size() - 1],
                                                       xOffset, yOffset, textureWidth, textureHeight,
                                                       stroke);
        xOffset += static_cast<int>(tempGlyph->BBox().Upper().X() - tempGlyph->BBox().Lower().X() + padding + 0.5);

        --remGlyphs;

        return tempGlyph;
    }


    void CalculateTextureSize()
    {
        if(!maximumGLTextureSize)
        {
            maximumGLTextureSize = 1024;
            glGetIntegerv(GL_MAX_TEXTURE_SIZE, (GLint*)&maximumGLTextureSize);
            assert(maximumGLTextureSize); // Indicates an invalid OpenGL context
        }

        // Texture width required for numGlyphs glyphs. Will probably not be
        // large enough, but we try to fit as many glyphs in one line as possible
        textureWidth = ClampSize(glyphWidth * numGlyphs + padding * 2,
                                 maximumGLTextureSize);

        // Number of lines required for that many glyphs in a line
        int tmp = (textureWidth - (padding * 2)) / glyphWidth;
        tmp = tmp > 0 ? tmp : 1;
        tmp = (numGlyphs + (tmp - 1)) / tmp; // round division up

        // Texture height required for tmp lines of glyphs
        textureHeight = ClampSize(glyphHeight * tmp + padding * 2,
                                  maximumGLTextureSize);
    }


    GLuint CreateTexture()
    {
        CalculateTextureSize();

        int totalMemory = textureWidth * textureHeight;
        unsigned char* textureMemory = new unsigned char[totalMemory];
        memset(textureMemory, 0, totalMemory);

        GLuint textID;
        glGenTextures(1, (GLuint*)&textID);

        glBindTexture(GL_TEXTURE_2D, textID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, textureWidth, textureHeight,
                     0, GL_ALPHA, GL_UNSIGNED_BYTE, textureMemory);

        delete [] textureMemory;

        return textID;
    }


    bool FaceSize(const unsigned int size, const unsigned int res)
    {
        if(!textureIDList.empty())
        {
            glDeleteTextures((GLsizei)textureIDList.size(), (const GLuint*)&textureIDList[0]);
            textureIDList.clear();
            remGlyphs = numGlyphs = face.GlyphCount();
        }

        return FTFont::FaceSize(size, res);
    }


    template <typename T>
    inline FTPoint RenderI(const T* string, const int len,
                                          FTPoint position, FTPoint spacing,
                                          int renderMode)
    {
        // Protect GL_TEXTURE_2D
        glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_TEXTURE_ENV_MODE);

        glEnable(GL_TEXTURE_2D);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

        FTTextureGlyph::ResetActiveTexture();

        FTPoint tmp = FTFont::Render(string, len,
                                     position, spacing, renderMode);

        glPopAttrib();

        return tmp;
    }


    FTPoint Render(const char * string, const int len,
                                      FTPoint position, FTPoint spacing,
                                      int renderMode)
    {
        return RenderI(string, len, position, spacing, renderMode);
    }


    FTPoint Render(const wchar_t * string, const int len,
                                      FTPoint position, FTPoint spacing,
                                      int renderMode)
    {
        return RenderI(string, len, position, spacing, renderMode);
    }
};

//
//  FTTextureGlyph
//

GLint FTTextureGlyph::activeTextureID = 0;

FTTextureGlyph::FTTextureGlyph(FT_GlyphSlot glyph, int id, int xOffset,
                               int yOffset, int width, int height, bool stroke)
:   FTGlyph(glyph),
    destWidth(0),
    destHeight(0),
    glTextureID(id)
{
    FT_Glyph actual_glyph;
    FT_Get_Glyph(glyph, &actual_glyph);

    if (stroke) {
        FT_Stroker stroker;
        FT_Stroker_New(*FTLibrary::Instance().GetLibrary(), &stroker);
        FT_Stroker_Set(stroker, 
            180,
            FT_STROKER_LINECAP_ROUND, 
            FT_STROKER_LINEJOIN_ROUND, 
            0
        );
        FT_Glyph_StrokeBorder(&actual_glyph, stroker, 0, 1);
        FT_Stroker_Done(stroker);
    }

    err = FT_Glyph_To_Bitmap(&actual_glyph, FT_RENDER_MODE_NORMAL, 0, 1);

    FT_BitmapGlyph bitmap_glyph = (FT_BitmapGlyph)actual_glyph;

/*    err = FT_Render_Glyph(glyph, FT_RENDER_MODE_NORMAL);*/

    if(err || actual_glyph->format != ft_glyph_format_bitmap)
    {
        return;
    }

    FT_Bitmap bitmap = bitmap_glyph->bitmap;

    destWidth  = bitmap.width;
    destHeight = bitmap.rows;

    if(destWidth && destHeight)
    {
        glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);

        glPixelStorei(GL_UNPACK_LSB_FIRST, GL_FALSE);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        GLint w, h;

        glBindTexture(GL_TEXTURE_2D, glTextureID);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);

        FTASSERT(xOffset >= 0);
        FTASSERT(yOffset >= 0);
        FTASSERT(destWidth >= 0);
        FTASSERT(destHeight >= 0);
        FTASSERT(xOffset + destWidth <= w);
        FTASSERT(yOffset + destHeight <= h);

        if (yOffset + destHeight > h)
        {
            // We'll only get here if we are soft-failing our asserts. In that
            // case, since the data we're trying to put into our texture is
            // too long, we'll only copy a portion of the image.
            destHeight = h - yOffset;
        }
        if (destHeight >= 0)
        {
            glTexSubImage2D(GL_TEXTURE_2D, 0, xOffset, yOffset,
                            destWidth, destHeight, GL_ALPHA, GL_UNSIGNED_BYTE,
                            bitmap.buffer);
        }

        glPopClientAttrib();
    }

//      0
//      +----+
//      |    |
//      |    |
//      |    |
//      +----+
//           1

    uv[0].X(static_cast<float>(xOffset) / static_cast<float>(width));
    uv[0].Y(static_cast<float>(yOffset) / static_cast<float>(height));
    uv[1].X(static_cast<float>(xOffset + destWidth) / static_cast<float>(width));
    uv[1].Y(static_cast<float>(yOffset + destHeight) / static_cast<float>(height));

    corner = FTPoint(bitmap_glyph->left, bitmap_glyph->top);

    FT_Done_Glyph(actual_glyph);
}


FTTextureGlyph::~FTTextureGlyph()
{}


const FTPoint& FTTextureGlyph::Render(const FTPoint& pen,
                                      int renderMode)
{
    float dx, dy;

    if(activeTextureID != glTextureID)
    {
        glBindTexture(GL_TEXTURE_2D, (GLuint)glTextureID);
        activeTextureID = glTextureID;
    }

    dx = floor(pen.Xf() + corner.Xf());
    dy = floor(pen.Yf() + corner.Yf());

    glBegin(GL_QUADS);
        glTexCoord2f(uv[0].Xf(), uv[0].Yf());
        glVertex3f(dx, dy, pen.Zf());

        glTexCoord2f(uv[0].Xf(), uv[1].Yf());
        glVertex3f(dx, dy - destHeight, pen.Zf());

        glTexCoord2f(uv[1].Xf(), uv[1].Yf());
        glVertex3f(dx + destWidth, dy - destHeight, pen.Zf());

        glTexCoord2f(uv[1].Xf(), uv[0].Yf());
        glVertex3f(dx + destWidth, dy, pen.Zf());
    glEnd();

    return advance;
}

// glyphcontainer

FTGlyphContainer::FTGlyphContainer(FTFace* f)
:   face(f),
    err(0)
{
    glyphs.push_back(NULL);
    charMap = new FTCharmap(face);
}


FTGlyphContainer::~FTGlyphContainer()
{
    GlyphVector::iterator it;
    for(it = glyphs.begin(); it != glyphs.end(); ++it)
    {
        delete *it;
    }

    glyphs.clear();
    delete charMap;
}


bool FTGlyphContainer::CharMap(FT_Encoding encoding)
{
    bool result = charMap->CharMap(encoding);
    err = charMap->Error();
    return result;
}


unsigned int FTGlyphContainer::FontIndex(const unsigned int charCode) const
{
    return charMap->FontIndex(charCode);
}


void FTGlyphContainer::Add(FTGlyph* tempGlyph, const unsigned int charCode)
{
    charMap->InsertIndex(charCode, glyphs.size());
    glyphs.push_back(tempGlyph);
}


const FTGlyph* const FTGlyphContainer::Glyph(const unsigned int charCode) const
{
    unsigned int index = charMap->GlyphListIndex(charCode);

    return (index < glyphs.size()) ? glyphs[index] : NULL;
}


FTBBox FTGlyphContainer::BBox(const unsigned int charCode) const
{
    return Glyph(charCode)->BBox();
}


float FTGlyphContainer::Advance(const unsigned int charCode,
                                const unsigned int nextCharCode)
{
    unsigned int left = charMap->FontIndex(charCode);
    unsigned int right = charMap->FontIndex(nextCharCode);
    const FTGlyph *glyph = Glyph(charCode);

    if (!glyph)
      return 0.0f;

    return face->KernAdvance(left, right).Xf() + glyph->Advance();
}


FTPoint FTGlyphContainer::Render(const unsigned int charCode,
                                 const unsigned int nextCharCode,
                                 FTPoint penPosition, int renderMode)
{
    unsigned int left = charMap->FontIndex(charCode);
    unsigned int right = charMap->FontIndex(nextCharCode);

    FTPoint kernAdvance = face->KernAdvance(left, right);

    if(!face->Error())
    {
        unsigned int index = charMap->GlyphListIndex(charCode);
        if (index < glyphs.size())
            kernAdvance += glyphs[index]->Render(penPosition, renderMode);
    }

    return kernAdvance;
}

// FTCharmap


FTCharmap::FTCharmap(FTFace* face)
:   ftFace(*(face->Face())),
    err(0)
{
    if(!ftFace->charmap)
    {
        if(!ftFace->num_charmaps)
        {
            // This face doesn't even have one charmap!
            err = 0x96; // Invalid_CharMap_Format
            return;
        }

        err = FT_Set_Charmap(ftFace, ftFace->charmaps[0]);
    }

    ftEncoding = ftFace->charmap->encoding;

    for(unsigned int i = 0; i < FTCharmap::MAX_PRECOMPUTED; i++)
    {
        charIndexCache[i] = FT_Get_Char_Index(ftFace, i);
    }
}


FTCharmap::~FTCharmap()
{
    charMap.clear();
}


bool FTCharmap::CharMap(FT_Encoding encoding)
{
    if(ftEncoding == encoding)
    {
        err = 0;
        return true;
    }

    err = FT_Select_Charmap(ftFace, encoding);

    if(!err)
    {
        ftEncoding = encoding;
        charMap.clear();
    }

    return !err;
}


unsigned int FTCharmap::GlyphListIndex(const unsigned int characterCode)
{
    return charMap.find(characterCode);
}


unsigned int FTCharmap::FontIndex(const unsigned int characterCode)
{
    if(characterCode < FTCharmap::MAX_PRECOMPUTED)
    {
        return charIndexCache[characterCode];
    }

    return FT_Get_Char_Index(ftFace, characterCode);
}


void FTCharmap::InsertIndex(const unsigned int characterCode,
                            const size_t containerIndex)
{
    charMap.insert(characterCode, static_cast<FTCharToGlyphIndexMap::GlyphIndex>(containerIndex));
}

// FTLayout

FTLayout::FTLayout() {}
FTLayout::~FTLayout() {}
FT_Error FTLayout::Error() const
{
    return err;
}

// FTSimpleLayout

FTSimpleLayout::FTSimpleLayout()
{
    currentFont = NULL;
    lineLength = 100.0f;
    alignment = ALIGN_LEFT;
    lineSpacing = 1.0f;
}


template <typename T>
inline FTBBox FTSimpleLayout::BBoxI(const T* string, const int len,
                                        FTPoint position)
{
    FTBBox tmp;

    WrapText(string, len, position, 0, &tmp);

    return tmp;
}


FTBBox FTSimpleLayout::BBox(const char *string, const int len,
                                FTPoint position)
{
    return BBoxI(string, len, position);
}


FTBBox FTSimpleLayout::BBox(const wchar_t *string, const int len,
                                FTPoint position)
{
    return BBoxI(string, len, position);
}


template <typename T>
inline void FTSimpleLayout::RenderI(const T *string, const int len,
                                    FTPoint position, int renderMode)
{
    pen = FTPoint(0.0f, 0.0f);
    WrapText(string, len, position, renderMode, NULL);
}


void FTSimpleLayout::Render(const char *string, const int len,
                            FTPoint position, int renderMode)
{
    RenderI(string, len, position, renderMode);
}


void FTSimpleLayout::Render(const wchar_t* string, const int len,
                                FTPoint position, int renderMode)
{
    RenderI(string, len, position, renderMode);
}


template <typename T>
inline void FTSimpleLayout::WrapTextI(const T *buf, const int len,
                                          FTPoint position, int renderMode,
                                          FTBBox *bounds)
{
    FTUnicodeStringItr<T> breakItr(buf);          // points to the last break character
    FTUnicodeStringItr<T> lineStart(buf);         // points to the line start
    float nextStart = 0.0;     // total width of the current line
    float breakWidth = 0.0;    // width of the line up to the last word break
    float currentWidth = 0.0;  // width of all characters on the current line
    float prevWidth;           // width of all characters but the current glyph
    float wordLength = 0.0;    // length of the block since the last break char
    int charCount = 0;         // number of characters so far on the line
    int breakCharCount = 0;    // number of characters before the breakItr
    float glyphWidth, advance;
    FTBBox glyphBounds;

    // Reset the pen position
    pen.Y(0);

    // If we have bounds mark them invalid
    if(bounds)
    {
        bounds->Invalidate();
    }

    // Scan the input for all characters that need output
    FTUnicodeStringItr<T> prevItr(buf);
    for (FTUnicodeStringItr<T> itr(buf); *itr; prevItr = itr++, charCount++)
    {
        // Find the width of the current glyph
        glyphBounds = currentFont->BBox(itr.getBufferFromHere(), 1);
        glyphWidth = glyphBounds.Upper().Xf() - glyphBounds.Lower().Xf();

        advance = currentFont->Advance(itr.getBufferFromHere(), 1);
        prevWidth = currentWidth;
        // Compute the width of all glyphs up to the end of buf[i]
        currentWidth = nextStart + glyphWidth;
        // Compute the position of the next glyph
        nextStart += advance;

        // See if the current character is a space, a break or a regular character
        if((currentWidth > lineLength) || (*itr == '\n'))
        {
            // A non whitespace character has exceeded the line length.  Or a
            // newline character has forced a line break.  Output the last
            // line and start a new line after the break character.
            // If we have not yet found a break, break on the last character
            if(breakItr == lineStart || (*itr == '\n'))
            {
                // Break on the previous character
                breakItr = prevItr;
                breakCharCount = charCount - 1;
                breakWidth = prevWidth;
                // None of the previous words will be carried to the next line
                wordLength = 0;
                // If the current character is a newline discard its advance
                if(*itr == '\n') advance = 0;
            }

            float remainingWidth = lineLength - breakWidth;

            // Render the current substring
            FTUnicodeStringItr<T> breakChar = breakItr;
            // move past the break character and don't count it on the next line either
            ++breakChar; --charCount;
            // If the break character is a newline do not render it
            if(*breakChar == '\n')
            {
                ++breakChar; --charCount;
            }

            OutputWrapped(lineStart.getBufferFromHere(), breakCharCount,
                          //breakItr.getBufferFromHere() - lineStart.getBufferFromHere(),
                          position, renderMode, remainingWidth, bounds);

            // Store the start of the next line
            lineStart = breakChar;
            // TODO: Is Height() the right value here?
            pen -= FTPoint(0, currentFont->LineHeight() * lineSpacing);
            // The current width is the width since the last break
            nextStart = wordLength + advance;
            wordLength += advance;
            currentWidth = wordLength + advance;
            // Reset the safe break for the next line
            breakItr = lineStart;
            charCount -= breakCharCount;
        }
        else if(iswspace(*itr))
        {
            // This is the last word break position
            wordLength = 0;
            breakItr = itr;
            breakCharCount = charCount;

            // Check to see if this is the first whitespace character in a run
            if(buf == itr.getBufferFromHere() || !iswspace(*prevItr))
            {
                // Record the width of the start of the block
                breakWidth = currentWidth;
            }
        }
        else
        {
            wordLength += advance;
        }
    }

    float remainingWidth = lineLength - currentWidth;
    // Render any remaining text on the last line
    // Disable justification for the last row
    if(alignment == ALIGN_JUSTIFY)
    {
        alignment = ALIGN_LEFT;
        OutputWrapped(lineStart.getBufferFromHere(), -1, position, renderMode,
                      remainingWidth, bounds);
        alignment = ALIGN_JUSTIFY;
    }
    else
    {
        OutputWrapped(lineStart.getBufferFromHere(), -1, position, renderMode,
                      remainingWidth, bounds);
    }
}


void FTSimpleLayout::WrapText(const char *buf, const int len,
                                  FTPoint position, int renderMode,
                                  FTBBox *bounds)
{
    WrapTextI(buf, len, position, renderMode, bounds);
}


void FTSimpleLayout::WrapText(const wchar_t* buf, const int len,
                                  FTPoint position, int renderMode,
                                  FTBBox *bounds)
{
    WrapTextI(buf, len, position, renderMode, bounds);
}


template <typename T>
inline void FTSimpleLayout::OutputWrappedI(const T *buf, const int len,
                                               FTPoint position, int renderMode,
                                               const float remaining,
                                               FTBBox *bounds)
{
    float distributeWidth = 0.0;
    // Align the text according as specified by Alignment
    switch (alignment)
    {
        case ALIGN_LEFT:
            pen.X(0);
            break;
        case ALIGN_CENTER:
            pen.X(remaining / 2);
            break;
        case ALIGN_RIGHT:
            pen.X(remaining);
            break;
        case ALIGN_JUSTIFY:
            pen.X(0);
            distributeWidth = remaining;
            break;
    }

    // If we have bounds expand them by the line's bounds, otherwise render
    // the line.
    if(bounds)
    {
        FTBBox temp = currentFont->BBox(buf, len);

        // Add the extra space to the upper x dimension
        temp = FTBBox(temp.Lower() + pen,
                      temp.Upper() + pen + FTPoint(distributeWidth, 0));

        // See if this is the first area to be added to the bounds
        if(bounds->IsValid())
        {
            *bounds |= temp;
        }
        else
        {
            *bounds = temp;
        }
    }
    else
    {
        RenderSpace(buf, len, position, renderMode, distributeWidth);
    }
}


void FTSimpleLayout::OutputWrapped(const char *buf, const int len,
                                       FTPoint position, int renderMode,
                                       const float remaining, FTBBox *bounds)
{
    OutputWrappedI(buf, len, position, renderMode, remaining, bounds);
}


void FTSimpleLayout::OutputWrapped(const wchar_t *buf, const int len,
                                       FTPoint position, int renderMode,
                                       const float remaining, FTBBox *bounds)
{
    OutputWrappedI(buf, len, position, renderMode, remaining, bounds);
}


template <typename T>
inline void FTSimpleLayout::RenderSpaceI(const T *string, const int len,
                                             FTPoint position, int renderMode,
                                             const float extraSpace)
{
    (void)position;

    float space = 0.0;

    // If there is space to distribute, count the number of spaces
    if(extraSpace > 0.0)
    {
        int numSpaces = 0;

        // Count the number of space blocks in the input
        FTUnicodeStringItr<T> prevItr(string), itr(string);
        for(int i = 0; ((len < 0) && *itr) || ((len >= 0) && (i <= len));
            ++i, prevItr = itr++)
        {
            // If this is the end of a space block, increment the counter
            if((i > 0) && !iswspace(*itr) && iswspace(*prevItr))
            {
                numSpaces++;
            }
        }

        space = extraSpace/numSpaces;
    }

    // Output all characters of the string
    FTUnicodeStringItr<T> prevItr(string), itr(string);
    for(int i = 0; ((len < 0) && *itr) || ((len >= 0) && (i <= len));
        ++i, prevItr = itr++)
    {
        // If this is the end of a space block, distribute the extra space
        // inside it
        if((i > 0) && !iswspace(*itr) && iswspace(*prevItr))
        {
            pen += FTPoint(space, 0);
        }

        pen = currentFont->Render(itr.getBufferFromHere(), 1, pen, FTPoint(), renderMode);
    }
}


void FTSimpleLayout::RenderSpace(const char *string, const int len,
                                     FTPoint position, int renderMode,
                                     const float extraSpace)
{
    RenderSpaceI(string, len, position, renderMode, extraSpace);
}


void FTSimpleLayout::RenderSpace(const wchar_t *string, const int len,
                                     FTPoint position, int renderMode,
                                     const float extraSpace)
{
    RenderSpaceI(string, len, position, renderMode, extraSpace);
}

void FTSimpleLayout::SetFont(FTFont *fontInit)
{
    currentFont = fontInit;
}


FTFont *FTSimpleLayout::GetFont()
{
    return currentFont;
}


void FTSimpleLayout::SetLineLength(const float LineLength)
{
    lineLength = LineLength;
}


float FTSimpleLayout::GetLineLength() const
{
    return lineLength;
}


void FTSimpleLayout::SetAlignment(const TextAlignment Alignment)
{
    alignment = Alignment;
}

TextAlignment FTSimpleLayout::GetAlignment() const
{
    return alignment;
}


void FTSimpleLayout::SetLineSpacing(const float LineSpacing)
{
    lineSpacing = LineSpacing;
}


float FTSimpleLayout::GetLineSpacing() const
{
    return lineSpacing;
}