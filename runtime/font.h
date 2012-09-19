#include "include_gl.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_TRUETYPE_TABLES_H
#include FT_OUTLINE_H
#include FT_STROKER_H
#include <set>

#define FTASSERT(x) \
    if (!(x)) \
    { \
        static int count = 0; \
        if (count++ < 8) \
            fprintf(stderr, "ASSERTION FAILED (%s:%d): %s\n", \
                    __FILE__, __LINE__, #x); \
        if (count == 8) \
            fprintf(stderr, "\\__ last warning for this assertion\n"); \
    }

typedef double   FTGL_DOUBLE;
typedef float    FTGL_FLOAT;

typedef enum
{
    RENDER_FRONT = 0x0001,
    RENDER_BACK  = 0x0002,
    RENDER_SIDE  = 0x0004,
    RENDER_ALL   = 0xffff
} RenderMode;

typedef enum
{
    ALIGN_LEFT    = 0,
    ALIGN_CENTER  = 1,
    ALIGN_RIGHT   = 2,
    ALIGN_JUSTIFY = 3
} TextAlignment;

template <typename FT_VECTOR_ITEM_TYPE>
class FTVector
{
    public:
        typedef FT_VECTOR_ITEM_TYPE value_type;
        typedef value_type& reference;
        typedef const value_type& const_reference;
        typedef value_type* iterator;
        typedef const value_type* const_iterator;
        typedef size_t size_type;

        FTVector()
        {
            Capacity = Size = 0;
            Items = 0;
        }


        virtual ~FTVector()
        {
            clear();
        }

        FTVector& operator =(const FTVector& v)
        {
            reserve(v.capacity());

            iterator ptr = begin();
            const_iterator vbegin = v.begin();
            const_iterator vend = v.end();

            while(vbegin != vend)
            {
                *ptr++ = *vbegin++;
            }

            Size = v.size();
            return *this;
        }

        size_type size() const
        {
            return Size;
        }

        size_type capacity() const
        {
            return Capacity;
        }

        iterator begin()
        {
            return Items;
        }

        const_iterator begin() const
        {
            return Items;
        }

        iterator end()
        {
            return begin() + size();
        }

        const_iterator end() const
        {
            return begin() + size();
        }

        bool empty() const
        {
            return size() == 0;
        }

        reference operator [](size_type pos)
        {
            return(*(begin() + pos));
        }

        const_reference operator [](size_type pos) const
        {
            return *(begin() + pos);
        }

        void clear()
        {
            if(Capacity)
            {
                delete [] Items;
                Capacity = Size = 0;
                Items = 0;
            }
        }

        void reserve(size_type n)
        {
            if(capacity() < n)
            {
                expand(n);
            }
        }

        void push_back(const value_type& x)
        {
            if(size() == capacity())
            {
                expand();
            }

           (*this)[size()] = x;
            ++Size;
        }

        void resize(size_type n, value_type x)
        {
            if(n == size())
            {
                return;
            }

            reserve(n);
            iterator ibegin, iend;

            if(n >= Size)
            {
                ibegin = this->end();
                iend = this->begin() + n;
            }
            else
            {
                ibegin = this->begin() + n;
                iend = this->end();
            }

            while(ibegin != iend)
            {
                *ibegin++ = x;
            }

            Size = n;
        }


    private:
        void expand(size_type capacity_hint = 0)
        {
            size_type new_capacity = (capacity() == 0) ? 256 : capacity() * 2;
            if(capacity_hint)
            {
                while(new_capacity < capacity_hint)
                {
                    new_capacity *= 2;
                }
            }

            value_type *new_items = new value_type[new_capacity];

            iterator ibegin = this->begin();
            iterator iend = this->end();
            value_type *ptr = new_items;

            while(ibegin != iend)
            {
                *ptr++ = *ibegin++;
            }

            if(Capacity)
            {
                delete [] Items;
            }

            Items = new_items;
            Capacity = new_capacity;
        }

        size_type Capacity;
        size_type Size;
        value_type* Items;
};

class FTPoint
{
    public:
        inline FTPoint()
        {
            values[0] = 0;
            values[1] = 0;
            values[2] = 0;
        }

        inline FTPoint(const FTGL_DOUBLE x, const FTGL_DOUBLE y,
                       const FTGL_DOUBLE z = 0)
        {
            values[0] = x;
            values[1] = y;
            values[2] = z;
        }

        inline FTPoint(const FT_Vector& ft_vector)
        {
            values[0] = ft_vector.x;
            values[1] = ft_vector.y;
            values[2] = 0;
        }

        FTPoint Normalise();
        inline FTPoint& operator += (const FTPoint& point)
        {
            values[0] += point.values[0];
            values[1] += point.values[1];
            values[2] += point.values[2];

            return *this;
        }

        inline FTPoint operator + (const FTPoint& point) const
        {
            FTPoint temp;
            temp.values[0] = values[0] + point.values[0];
            temp.values[1] = values[1] + point.values[1];
            temp.values[2] = values[2] + point.values[2];

            return temp;
        }

        inline FTPoint& operator -= (const FTPoint& point)
        {
            values[0] -= point.values[0];
            values[1] -= point.values[1];
            values[2] -= point.values[2];

            return *this;
        }

        inline FTPoint operator - (const FTPoint& point) const
        {
            FTPoint temp;
            temp.values[0] = values[0] - point.values[0];
            temp.values[1] = values[1] - point.values[1];
            temp.values[2] = values[2] - point.values[2];

            return temp;
        }

        inline FTPoint operator * (double multiplier) const
        {
            FTPoint temp;
            temp.values[0] = values[0] * multiplier;
            temp.values[1] = values[1] * multiplier;
            temp.values[2] = values[2] * multiplier;

            return temp;
        }


        inline friend FTPoint operator * (double multiplier, FTPoint& point)
        {
            return point * multiplier;
        }

        inline friend double operator * (FTPoint &a, FTPoint& b)
        {
            return a.values[0] * b.values[0]
                 + a.values[1] * b.values[1]
                 + a.values[2] * b.values[2];
        }

        inline FTPoint operator ^ (const FTPoint& point)
        {
            FTPoint temp;
            temp.values[0] = values[1] * point.values[2]
                              - values[2] * point.values[1];
            temp.values[1] = values[2] * point.values[0]
                              - values[0] * point.values[2];
            temp.values[2] = values[0] * point.values[1]
                              - values[1] * point.values[0];
            return temp;
        }

        friend bool operator == (const FTPoint &a, const FTPoint &b);
        friend bool operator != (const FTPoint &a, const FTPoint &b);

        inline operator const FTGL_DOUBLE*() const
        {
            return values;
        }

        inline void X(FTGL_DOUBLE x) { values[0] = x; };
        inline void Y(FTGL_DOUBLE y) { values[1] = y; };
        inline void Z(FTGL_DOUBLE z) { values[2] = z; };

        inline FTGL_DOUBLE X() const { return values[0]; };
        inline FTGL_DOUBLE Y() const { return values[1]; };
        inline FTGL_DOUBLE Z() const { return values[2]; };
        inline FTGL_FLOAT Xf() const { return static_cast<FTGL_FLOAT>(values[0]); };
        inline FTGL_FLOAT Yf() const { return static_cast<FTGL_FLOAT>(values[1]); };
        inline FTGL_FLOAT Zf() const { return static_cast<FTGL_FLOAT>(values[2]); };

    private:
        FTGL_DOUBLE values[3];
};

class FTBBox
{
    public:
        FTBBox()
        :   lower(0.0f, 0.0f, 0.0f),
            upper(0.0f, 0.0f, 0.0f)
        {}

        FTBBox(float lx, float ly, float lz, float ux, float uy, float uz)
        :   lower(lx, ly, lz),
            upper(ux, uy, uz)
        {}

        FTBBox(FTPoint l, FTPoint u)
        :   lower(l),
            upper(u)
        {}

        FTBBox(FT_GlyphSlot glyph)
        :   lower(0.0f, 0.0f, 0.0f),
            upper(0.0f, 0.0f, 0.0f)
        {
            FT_BBox bbox;
            FT_Outline_Get_CBox(&(glyph->outline), &bbox);

            lower.X(static_cast<float>(bbox.xMin) / 64.0f);
            lower.Y(static_cast<float>(bbox.yMin) / 64.0f);
            lower.Z(0.0f);
            upper.X(static_cast<float>(bbox.xMax) / 64.0f);
            upper.Y(static_cast<float>(bbox.yMax) / 64.0f);
            upper.Z(0.0f);
        }

        ~FTBBox()
        {}

        void Invalidate()
        {
            lower = FTPoint(1.0f, 1.0f, 1.0f);
            upper = FTPoint(-1.0f, -1.0f, -1.0f);
        }

        bool IsValid()
        {
            return lower.X() <= upper.X()
                && lower.Y() <= upper.Y()
                && lower.Z() <= upper.Z();
        }


        FTBBox& operator += (const FTPoint vector)
        {
            lower += vector;
            upper += vector;

            return *this;
        }

        FTBBox& operator |= (const FTBBox& bbox)
        {
            if(bbox.lower.X() < lower.X()) lower.X(bbox.lower.X());
            if(bbox.lower.Y() < lower.Y()) lower.Y(bbox.lower.Y());
            if(bbox.lower.Z() < lower.Z()) lower.Z(bbox.lower.Z());
            if(bbox.upper.X() > upper.X()) upper.X(bbox.upper.X());
            if(bbox.upper.Y() > upper.Y()) upper.Y(bbox.upper.Y());
            if(bbox.upper.Z() > upper.Z()) upper.Z(bbox.upper.Z());

            return *this;
        }

        void SetDepth(float depth)
        {
            if(depth > 0)
                upper.Z(lower.Z() + depth);
            else
                lower.Z(upper.Z() + depth);
        }


        inline FTPoint const Upper() const
        {
            return upper;
        }


        inline FTPoint const Lower() const
        {
            return lower;
        }

    private:
        FTPoint lower, upper;
};

class FTLibrary
{
    public:
        static const FTLibrary& Instance();
        const FT_Library* const GetLibrary() const { return library; }
        FT_Error Error() const { return err; }
        ~FTLibrary();

    private:
        FTLibrary();
        FTLibrary(const FT_Library&){}
        FTLibrary& operator=(const FT_Library&) { return *this; }
        bool Initialise();
        FT_Library* library;
        FT_Error err;
};

class FTSize
{
    public:
        FTSize();
        virtual ~FTSize();

        bool CharSize(FT_Face* face, unsigned int point_size,
                      unsigned int x_resolution, unsigned int y_resolution);
        unsigned int CharSize() const;
        float Ascender() const;
        float Descender() const;
        float Height() const;
        float Width() const;
        float Underline() const;
        FT_Error Error() const { return err; }

    private:
        FT_Face* ftFace;
        FT_Size ftSize;
        unsigned int size;
        unsigned int xResolution;
        unsigned int yResolution;
        FT_Error err;
};

class FTFace
{
    public:
        FTFace(const char* fontFilePath, bool precomputeKerning = true);
        FTFace(const unsigned char *pBufferBytes, size_t bufferSizeInBytes,
               bool precomputeKerning = true);
        virtual ~FTFace();
        bool Attach(const char* fontFilePath);
        bool Attach(const unsigned char *pBufferBytes,
                    size_t bufferSizeInBytes);
        FT_Face* Face() const { return ftFace; }
        const FTSize& Size(const unsigned int size, const unsigned int res);
        unsigned int CharMapCount() const;
        FT_Encoding* CharMapList();
        FTPoint KernAdvance(unsigned int index1, unsigned int index2);
        FT_GlyphSlot Glyph(unsigned int index, FT_Int load_flags);
        unsigned int GlyphCount() const { return numGlyphs; }
        FT_Error Error() const { return err; }

    private:
        FT_Face* ftFace;
        FTSize  charSize;
        int numGlyphs;
        FT_Encoding* fontEncodingList;
        bool hasKerningTable;
        void BuildKerningCache();
        static const unsigned int MAX_PRECOMPUTED = 128;
        FTGL_DOUBLE* kerningCache;
        FT_Error err;
};

class FTGlyph
{
    public:
        FTGlyph(FT_GlyphSlot glyph);
        virtual ~FTGlyph();
        virtual const FTPoint& Render(const FTPoint& pen, int renderMode) = 0;
        virtual float Advance() const;
        virtual const FTBBox& BBox() const;
        virtual FT_Error Error() const;

        FTPoint advance;
        FTBBox bBox;
        FT_Error err;
};

class FTCharToGlyphIndexMap
{
    public:
        typedef unsigned long CharacterCode;
        typedef signed long GlyphIndex;

        // XXX: always ensure that 1 << (3 * BucketIdxBits) >= UnicodeValLimit
        static const int BucketIdxBits = 7;
        static const int BucketIdxSize = 1 << BucketIdxBits;
        static const int BucketIdxMask = BucketIdxSize - 1;

        static const CharacterCode UnicodeValLimit = 0x110000;
        static const int IndexNotFound = -1;

        FTCharToGlyphIndexMap()
        {
            Indices = 0;
        }

        virtual ~FTCharToGlyphIndexMap()
        {
            // Free all buckets
            clear();
        }

        inline void clear()
        {
            for(int j = 0; Indices && j < BucketIdxSize; j++)
            {
                for(int i = 0; Indices[j] && i < BucketIdxSize; i++)
                {
                    delete[] Indices[j][i];
                    Indices[j][i] = 0;
                }
                delete[] Indices[j];
                Indices[j] = 0;
            }
            delete[] Indices;
            Indices = 0;
        }

        const GlyphIndex find(CharacterCode c)
        {
            int OuterIdx = (c >> (BucketIdxBits * 2)) & BucketIdxMask;
            int InnerIdx = (c >> BucketIdxBits) & BucketIdxMask;
            int Offset = c & BucketIdxMask;

            if (c >= UnicodeValLimit || !Indices
                 || !Indices[OuterIdx] || !Indices[OuterIdx][InnerIdx])
                return 0;

            GlyphIndex g = Indices[OuterIdx][InnerIdx][Offset];

            return (g != IndexNotFound) ? g : 0;
        }

        void insert(CharacterCode c, GlyphIndex g)
        {
            int OuterIdx = (c >> (BucketIdxBits * 2)) & BucketIdxMask;
            int InnerIdx = (c >> BucketIdxBits) & BucketIdxMask;
            int Offset = c & BucketIdxMask;

            if (c >= UnicodeValLimit)
                return;

            if (!Indices)
            {
                Indices = new GlyphIndex** [BucketIdxSize];
                for(int i = 0; i < BucketIdxSize; i++)
                    Indices[i] = 0;
            }

            if (!Indices[OuterIdx])
            {
                Indices[OuterIdx] = new GlyphIndex* [BucketIdxSize];
                for(int i = 0; i < BucketIdxSize; i++)
                    Indices[OuterIdx][i] = 0;
            }

            if (!Indices[OuterIdx][InnerIdx])
            {
                Indices[OuterIdx][InnerIdx] = new GlyphIndex [BucketIdxSize];
                for(int i = 0; i < BucketIdxSize; i++)
                    Indices[OuterIdx][InnerIdx][i] = IndexNotFound;
            }

            Indices[OuterIdx][InnerIdx][Offset] = g;
        }

    private:
        GlyphIndex*** Indices;
};

class FTCharmap
{
    public:
        FTCharmap(FTFace* face);
        virtual ~FTCharmap();
        FT_Encoding Encoding() const { return ftEncoding; }
        bool CharMap(FT_Encoding encoding);
        unsigned int GlyphListIndex(const unsigned int characterCode);
        unsigned int FontIndex(const unsigned int characterCode);
        void InsertIndex(const unsigned int characterCode,
                         const size_t containerIndex);
        FT_Error Error() const { return err; }

    private:
        FT_Encoding ftEncoding;
        const FT_Face ftFace;
        typedef FTCharToGlyphIndexMap CharacterMap;
        CharacterMap charMap;
        static const unsigned int MAX_PRECOMPUTED = 128;
        unsigned int charIndexCache[MAX_PRECOMPUTED];
        FT_Error err;
};

class FTGlyphContainer
{
        typedef FTVector<FTGlyph*> GlyphVector;
    public:
        FTGlyphContainer(FTFace* face);
        ~FTGlyphContainer();
        bool CharMap(FT_Encoding encoding);
        unsigned int FontIndex(const unsigned int characterCode) const;
        void Add(FTGlyph* glyph, const unsigned int characterCode);
        const FTGlyph* const Glyph(const unsigned int characterCode) const;
        FTBBox BBox(const unsigned int characterCode) const;
        float Advance(const unsigned int characterCode,
                      const unsigned int nextCharacterCode);
        FTPoint Render(const unsigned int characterCode,
                       const unsigned int nextCharacterCode,
                       FTPoint penPosition, int renderMode);
        FT_Error Error() const { return err; }

    private:
        FTFace* face;
        FTCharmap* charMap;
        GlyphVector glyphs;
        FT_Error err;
};

class FTFont
{
public:
    FTFont(char const *fontFilePath);

    virtual ~FTFont();
    virtual void GlyphLoadFlags(FT_Int flags);
    virtual bool CharMap(FT_Encoding encoding);
    virtual unsigned int CharMapCount() const;
    virtual FT_Encoding* CharMapList();
    virtual float Ascender() const;
    virtual float Descender() const;
    virtual float LineHeight() const;
    virtual bool FaceSize(const unsigned int size,
                          const unsigned int res);
    virtual unsigned int FaceSize() const;
    virtual FTBBox BBox(const char *s, const int len = -1, 
                        FTPoint position = FTPoint(), 
                        FTPoint spacing = FTPoint());
    virtual FTBBox BBox(const wchar_t *s, const int len = -1, 
                        FTPoint position = FTPoint(), 
                        FTPoint spacing = FTPoint());
    virtual float Advance(const char *s, const int len = -1,
                          FTPoint spacing = FTPoint());
    virtual float Advance(const wchar_t *s, const int len = -1,
                          FTPoint spacing = FTPoint());
    virtual FTPoint Render(const char *s, const int len,
                           FTPoint, FTPoint, int);
    virtual FTPoint Render(const wchar_t *s, const int len,
                           FTPoint, FTPoint, int);
    virtual FTGlyph* MakeGlyph(FT_GlyphSlot ftGlyph) = 0;

    FTFace face;
    FTSize charSize;
    FT_Int load_flags;
    FT_Error err;
    bool CheckGlyph(const unsigned int chr);
    FTGlyphContainer* glyphList;
    FTPoint pen;

    template <typename T>
    inline FTBBox BBoxI(const T *s, const int len,
                        FTPoint position, FTPoint spacing);

    template <typename T>
    inline float AdvanceI(const T *s, const int len, FTPoint spacing);

    template <typename T>
    inline FTPoint RenderI(const T *s, const int len,
                           FTPoint position, FTPoint spacing, int mode);
};

typedef void* FTCleanupObject;
class FTCleanup
{
    protected:
        static FTCleanup* _instance;

        FTCleanup::FTCleanup()
        {
        }


        ~FTCleanup()
        {
            std::set<FT_Face **>::iterator cleanupItr = cleanupFT_FaceItems.begin();
            FT_Face **cleanupFace = 0;

            while (cleanupItr != cleanupFT_FaceItems.end())
            {
                cleanupFace = *cleanupItr;
                if (*cleanupFace)
                {
                    FT_Done_Face(**cleanupFace);
                    delete *cleanupFace;
                    *cleanupFace = 0;
                }
                cleanupItr++;
            }
            cleanupFT_FaceItems.clear();
        }

    public:

        static FTCleanup* Instance()
        {
            if (FTCleanup::_instance == 0)
                FTCleanup::_instance = new FTCleanup;
            return FTCleanup::_instance;
        }

        static void DestroyAll()
        {
            delete FTCleanup::_instance;
        }


        void RegisterObject(FT_Face **obj)
        {
            cleanupFT_FaceItems.insert(obj);
        }


        void UnregisterObject(FT_Face **obj)
        {
            cleanupFT_FaceItems.erase(obj);
        }

    private:
        std::set<FT_Face **> cleanupFT_FaceItems;
};

FTCleanup * FTCleanup::_instance = 0;

class FTTextureGlyph : public FTGlyph
{
    public:
        FTTextureGlyph(FT_GlyphSlot glyph, int id, int xOffset,
                       int yOffset, int width, int height, bool stroke);

        virtual ~FTTextureGlyph();
        virtual const FTPoint& Render(const FTPoint& pen, int renderMode);
        static void ResetActiveTexture() { activeTextureID = 0; }

    private:
        int destWidth;
        int destHeight;
        FTPoint corner;
        FTPoint uv[2];
        int glTextureID;
        static GLint activeTextureID;
};

class FTLayout
{
    public:
        FTLayout();
        virtual ~FTLayout();
        virtual FTBBox BBox(const char* string, const int len = -1,
                            FTPoint position = FTPoint()) = 0;
        virtual FTBBox BBox(const wchar_t* string, const int len = -1,
                            FTPoint position = FTPoint()) = 0;
        virtual void Render(const char *string, const int len = -1,
                            FTPoint position = FTPoint(),
                            int renderMode = RENDER_ALL) = 0;
        virtual void Render(const wchar_t *string, const int len = -1,
                            FTPoint position = FTPoint(),
                            int renderMode = RENDER_ALL) = 0;
        virtual FT_Error Error() const;

        FTPoint pen;
        FT_Error err;
};

class FTSimpleLayout : public FTLayout
{
    public:
        FTSimpleLayout();
        virtual FTBBox BBox(const char* string, const int len = -1,
                            FTPoint position = FTPoint());
        virtual FTBBox BBox(const wchar_t* string, const int len = -1,
                            FTPoint position = FTPoint());
        virtual void Render(const char *string, const int len = -1,
                            FTPoint position = FTPoint(),
                            int renderMode = RENDER_ALL);
        virtual void Render(const wchar_t *string, const int len = -1,
                            FTPoint position = FTPoint(),
                            int renderMode = RENDER_ALL);
        void SetFont(FTFont *fontInit);
        FTFont *GetFont();
        void SetLineLength(const float LineLength);
        float GetLineLength() const;
        void SetAlignment(const TextAlignment Alignment);
        TextAlignment GetAlignment() const;
        void SetLineSpacing(const float LineSpacing);
        float GetLineSpacing() const;
        virtual void RenderSpace(const char *string, const int len,
                                 FTPoint position, int renderMode,
                                 const float extraSpace);
        virtual void RenderSpace(const wchar_t *string, const int len,
                                 FTPoint position, int renderMode,
                                 const float extraSpace);

    private:
        FTFont *currentFont;
        float lineLength;
        TextAlignment alignment;
        float lineSpacing;

        virtual void WrapText(const char *buf, const int len,
                              FTPoint position, int renderMode,
                              FTBBox *bounds);
        virtual void WrapText(const wchar_t *buf, const int len,
                              FTPoint position, int renderMode,
                              FTBBox *bounds);
        void OutputWrapped(const char *buf, const int len,
                           FTPoint position, int renderMode,
                           const float RemainingWidth, FTBBox *bounds);
        void OutputWrapped(const wchar_t *buf, const int len,
                           FTPoint position, int renderMode,
                           const float RemainingWidth, FTBBox *bounds);

        template <typename T>
        inline FTBBox BBoxI(const T* string, const int len, FTPoint position);

        template <typename T>
        inline void RenderI(const T* string, const int len,
                            FTPoint position, int renderMode);

        template <typename T>
        inline void RenderSpaceI(const T* string, const int len,
                                 FTPoint position, int renderMode,
                                 const float extraSpace);
        template <typename T>
        void WrapTextI(const T* buf, const int len, FTPoint position,
                       int renderMode, FTBBox *bounds);

        template <typename T>
        void OutputWrappedI(const T* buf, const int len, FTPoint position,
                            int renderMode, const float RemainingWidth,
                            FTBBox *bounds);


};

// unicode support

template <typename T>
class FTUnicodeStringItr
{
public:
    /**
     * Constructor.  Also reads the first character and stores it.
     *
     * @param string  The buffer to iterate.  No copy is made.
     */
    FTUnicodeStringItr(const T* string) : curPos(string), nextPos(string)
    {
        (*this)++;
    };

    /**
     * Pre-increment operator.  Reads the next unicode character and sets
     * the state appropriately.
     * Note - not protected against overruns.
     */
    FTUnicodeStringItr& operator++()
    {
        curPos = nextPos;
        // unicode handling
        switch (sizeof(T))
        {
            case 1: // UTF-8
                // get this character
                readUTF8(); break;
            case 2: // UTF-16
                readUTF16(); break;
            case 4: // UTF-32
                // fall through
            default: // error condition really, but give it a shot anyway
                curChar = *nextPos++;
        }
        return *this;
    }

    /**
     * Post-increment operator.  Reads the next character and sets
     * the state appropriately.
     * Note - not protected against overruns.
     */
    FTUnicodeStringItr operator++(int)
    {
        FTUnicodeStringItr temp = *this;
        ++*this;
        return temp;
    }

    /**
     * Equality operator.  Two FTUnicodeStringItrs are considered equal
     * if they have the same current buffer and buffer position.
     */
    bool operator==(const FTUnicodeStringItr& right) const
    {
        if (curPos == right.getBufferFromHere())
            return true;
        return false;
    }

    /**
     * Dereference operator.
     *
     * @return  The unicode codepoint of the character currently pointed
     * to by the FTUnicodeStringItr.
     */
    unsigned int operator*() const
    {
        return curChar;
    }

    /**
     * Buffer-fetching getter.  You can use this to retreive the buffer
     * starting at the currently-iterated character for functions which
     * require a Unicode string as input.
     */
    const T* getBufferFromHere() const { return curPos; }

private:
    /**
     * Helper function for reading a single UTF8 character from the string.
     * Updates internal state appropriately.
     */
    void readUTF8();

    /**
     * Helper function for reading a single UTF16 character from the string.
     * Updates internal state appropriately.
     */
    void readUTF16();

    /**
     * The buffer position of the first element in the current character.
     */
    const T* curPos;

    /**
     * The character stored at the current buffer position (prefetched on
     * increment, so there's no penalty for dereferencing more than once).
     */
    unsigned int curChar;

    /**
     * The buffer position of the first element in the next character.
     */
    const T* nextPos;

    // unicode magic numbers
    static const unsigned char utf8bytes[256];
    static const unsigned long offsetsFromUTF8[6];
    static const unsigned long highSurrogateStart;
    static const unsigned long highSurrogateEnd;
    static const unsigned long lowSurrogateStart;
    static const unsigned long lowSurrogateEnd;
    static const unsigned long highSurrogateShift;
    static const unsigned long lowSurrogateBase;
};

/* The first character in a UTF8 sequence indicates how many bytes
 * to read (among other things) */
template <typename T>
const unsigned char FTUnicodeStringItr<T>::utf8bytes[256] = {
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3, 4,4,4,4,4,4,4,4,5,5,5,5,6,6,6,6
};

/* Magic values subtracted from a buffer value during UTF8 conversion.
 * This table contains as many values as there might be trailing bytes
 * in a UTF-8 sequence. */
template <typename T>
const unsigned long FTUnicodeStringItr<T>::offsetsFromUTF8[6] = { 0x00000000UL, 0x00003080UL, 0x000E2080UL,
  0x03C82080UL, 0xFA082080UL, 0x82082080UL };

// get a UTF8 character; leave the tracking pointer at the start of the
// next character
// not protected against invalid UTF8
template <typename T>
inline void FTUnicodeStringItr<T>::readUTF8()
{
    unsigned int ch = 0;
    unsigned int extraBytesToRead = utf8bytes[(unsigned char)(*nextPos)];
    // falls through
    switch (extraBytesToRead)
    {
          case 6: ch += *nextPos++; ch <<= 6; /* remember, illegal UTF-8 */
          case 5: ch += *nextPos++; ch <<= 6; /* remember, illegal UTF-8 */
          case 4: ch += *nextPos++; ch <<= 6;
          case 3: ch += *nextPos++; ch <<= 6;
          case 2: ch += *nextPos++; ch <<= 6;
          case 1: ch += *nextPos++;
    }
    ch -= offsetsFromUTF8[extraBytesToRead-1];
    curChar = ch;
}

// Magic numbers for UTF-16 conversions
template <typename T>
const unsigned long FTUnicodeStringItr<T>::highSurrogateStart = 0xD800;
template <typename T>
const unsigned long FTUnicodeStringItr<T>::highSurrogateEnd = 0xDBFF;
template <typename T>
const unsigned long FTUnicodeStringItr<T>::lowSurrogateStart = 0xDC00;
template <typename T>
const unsigned long FTUnicodeStringItr<T>::lowSurrogateEnd = 0xDFFF;
template <typename T>
const unsigned long FTUnicodeStringItr<T>::highSurrogateShift = 10;
template <typename T>
const unsigned long FTUnicodeStringItr<T>::lowSurrogateBase = 0x0010000UL;

template <typename T>
inline void FTUnicodeStringItr<T>::readUTF16()
{
    unsigned int ch = *nextPos++;
    // if we have the first half of the surrogate pair
    if (ch >= highSurrogateStart && ch <= highSurrogateEnd)
    {
        unsigned int ch2 = *curPos;
        // complete the surrogate pair
        if (ch2 >= lowSurrogateStart && ch2 <= lowSurrogateEnd)
        {
            ch = ((ch - highSurrogateStart) << highSurrogateShift)
                + (ch2 - lowSurrogateStart) + lowSurrogateBase;
            ++nextPos;
        }
    }
    curChar = ch;
}