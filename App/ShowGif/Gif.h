

#pragma pack (push, 1)

typedef struct {
  UINT8         Signature[3];
  UINT8         Version[3];
  UINT16        Width;
  UINT16        Height;
  UINT8         GctSize     : 3;
  UINT8         SortFlag    : 1;
  UINT8         ColorRes    : 3;
  UINT8         GctFlag     : 1;
  UINT8         BackgroundColorIndex;
  UINT8         PixelAspectRatio;
} GIF_HEADER;

typedef struct {
  UINT8         Red;
  UINT8         Green;
  UINT8         Blue;
} GIF_COLOR;

typedef struct {
  UINT8         Separator;
  UINT16        Left;
  UINT16        Top;
  UINT16        Width;
  UINT16        Height;
  UINT8         LctSize         : 3;
  UINT8         Reserved        : 2;
  UINT8         SortFlag        : 1;
  UINT8         InterlaceFlag   : 1;
  UINT8         LctFlage        : 1;
} GIF_IMAGE_DESCRIPTOR;

typedef struct {
  UINT8         LzwCodeSize;
  UINT8         ImageData;
} GIF_TABLE_BASED_IMAGE_DATA;

typedef struct {
  UINT8                 Terminator;
} GIF_BLOCK_TERMINATOR;

typedef struct {
  UINT8         Introducer;
  UINT8         Label;
} GIF_EXTENSION_HEADER;

typedef struct {
  GIF_EXTENSION_HEADER  Header;
  UINT8                 Size;
  UINT8                 TransparentColorFlag : 1;
  UINT8                 UserInputFlag        : 1;
  UINT8                 DisposalMethod       : 3;
  UINT8                 Reserved             : 3;
  UINT16                DelayTime;
  UINT8                 TransparentColorIndex;
} GIF_GRAPHIC_CONTROL_EXTENSION;

typedef struct {
  GIF_EXTENSION_HEADER  Header;
  UINT8                 Size;
  UINT8                 Data[];
} GIF_COMMENT_EXTENSION;

typedef struct {
  GIF_EXTENSION_HEADER  Header;
  UINT8                 Size;
  UINT16                GridLeft;
  UINT16                GridTop;
  UINT16                GridWidth;
  UINT16                GridHeight;
  UINT8                 CellWidth;
  UINT8                 CellHeight;
  UINT8                 ForegroundColorIndex;
  UINT8                 BackgroundColorIndex;
  UINT8                 Data[];
} GIF_PLAIN_TEXT_EXTENSION;

typedef struct {
  GIF_EXTENSION_HEADER  Header;
  UINT8                 Size;
  UINT8                 Identifier[11];
  UINT8                 SubSize;
  UINT8                 Id;
  UINT16                LoopCount;
} GIF_APPLICATION_EXTENSION;

typedef struct {
  UINT8                 Trailer;
} GIF_TRAILER;

#pragma pack (pop)