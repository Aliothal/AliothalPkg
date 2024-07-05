#include <Library/ShellLib.h>
#include <Library/ShellCommandLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/FileHandleLib.h>
#include <Library/PrintLib.h>

#define JSON_READ8(Json)              ((Json)->Buffer[(Json)->Offset])
#define JSON_READ8_PLUS(Json)         ((Json)->Buffer[(Json)->Offset++])
#define JSON_INC(Json)                ((Json)->Offset++)
#define JSON_DEC(Json)                ((Json)->Offset--)
#define IS_0_9(ch)                    ((ch) >= '0' && (ch) <= '9')
#define IS_1_9(ch)                    ((ch) >= '1' && (ch) <= '9')
#define JSON_INIT_VALUE(JsonValue)    do {                                                    \
                                        (JsonValue) = AllocateZeroPool(sizeof(JSON_VALUE));   \
                                        InitializeListHead(&(JsonValue)->Item.Link);          \
                                      } while (0)
//
#define JSON_STRING_BUFFER_SIZE SIZE_1KB

#define JSON_RETURN_PARSE_OK    0x00
#define JSON_RETURN_STRING_END  0x01
#define JSON_RETURN_PARSE_ERROR 0x02

CHAR16 Result[0x100];
typedef struct {
  const CHAR8 *Buffer;
  const UINTN Length;
  UINTN       Offset;

} JSON_CONTENT;

typedef enum {
  JsonValueUnkown,
  JsonValueObject,
  JsonValueArray,
  JsonValueString,
  JsonValueNumber,
  JsonValueTrue,
  JsonValueFalse,
  JsonValueNull
} JsonValueType;

typedef struct _JSON_VALUE JSON_VALUE;

// typedef struct {
//   double              Number;
// } JSON_NUMBER;

// typedef struct {
//   CHAR8               *True;
//   UINTN               Size;
// } JSON_TRUE;

// typedef struct {
//   CHAR8               *False;
//   UINTN               Size;
// } JSON_FALSE;

// typedef struct {
//   CHAR8               *Null;
//   UINTN               Size;
// } JSON_NULL;

typedef struct {
  CHAR8               *String;
  UINTN               Size;
} JSON_STRING;

typedef struct {
  LIST_ENTRY          Link;
  JSON_STRING         Key;
  JSON_VALUE          *Value;
} JSON_OBJECT;

typedef struct {
  LIST_ENTRY          Link;
  UINTN               Index;
  JSON_VALUE          *Value;
} JSON_ARRAY;

struct _JSON_VALUE {
  union {
    LIST_ENTRY        Link;
    JSON_OBJECT       Object;
    JSON_ARRAY        Array;
    JSON_STRING       String;
  } Item;
  JsonValueType       Type;
};

UINT8 ParseStringLiteral(JSON_CONTENT *Json, CHAR8 **String, UINTN *StrSize);
UINT8 ParseValue(JSON_CONTENT *Json, JSON_VALUE *JsonRoot);

void TrimSpaces(JSON_CONTENT *Json)
{
  while (TRUE) {
    switch (JSON_READ8(Json)) {
      case '\t':
      case '\n':
      case '\r':
      case ' ':
        JSON_INC(Json);
        break;
      default:
        return;
    }
  }
}

void FreeJson(JSON_VALUE *JsonRoot) {
  if (JsonRoot == NULL)
    return;
  switch (JsonRoot->Type)
  {
    case JsonValueObject:
      Print(L"Free obj\n");
      if (JsonRoot->Item.Object.Key.Size != 0) {
        JsonRoot->Item.Object.Key.Size = 0;
        FreePool(JsonRoot->Item.Object.Key.String);
        JsonRoot->Item.Object.Key.String = NULL;
      }
      if (JsonRoot->Item.Object.Value != NULL) {
        for (LIST_ENTRY *Entry = JsonRoot->Item.Link.BackLink; Entry != &JsonRoot->Item.Link; Entry = Entry->BackLink) {
          // JSON_OBJECT *obj = BASE_CR(Entry, JSON_OBJECT, Link);
          JSON_VALUE *JsonItem = BASE_CR(Entry, JSON_VALUE, Item.Link);
          if (JsonItem->Item.Object.Key.Size != 0) {
            JsonItem->Item.Object.Key.Size = 0;
            FreePool(JsonItem->Item.Object.Key.String);
            JsonItem->Item.Object.Key.String = NULL;
          }
          FreeJson(JsonItem->Item.Object.Value);
          FreePool(JsonItem);
          JsonItem = NULL;
        }
        FreeJson(JsonRoot->Item.Object.Value);
        FreePool(JsonRoot->Item.Object.Value);
        JsonRoot->Item.Object.Value = NULL;
      }
      FreePool(JsonRoot);
      JsonRoot= NULL;
      break;
    case JsonValueArray:
    case JsonValueString:
    case JsonValueNumber:
    case JsonValueTrue:
    case JsonValueFalse:
    case JsonValueNull:
      Print(L"Free str\n");
      if (JsonRoot->Item.String.Size != 0) {
        FreePool(JsonRoot->Item.String.String);
        JsonRoot->Item.String.String = NULL;
      }
      break;
    case JsonValueUnkown:
    default :
      break;
  }
}


UINT8 ParseHex()
{
  //todo
}

UINT8 ParseStringLiteral(JSON_CONTENT *Json, CHAR8 **String, UINTN *StrSize)
{
  JSON_INC(Json);
  CHAR8     *Buffer = AllocateZeroPool(JSON_STRING_BUFFER_SIZE);
  UINTN     Size = 0;
  while (TRUE) {
    switch (JSON_READ8(Json)) {
      case '\"':
        JSON_INC(Json);
        *String = AllocateCopyPool(Size+1, Buffer);
        *StrSize = Size+1;
        FreePool(Buffer);
        return JSON_RETURN_PARSE_OK;
      case '\\':
        JSON_INC(Json);
        switch (JSON_READ8(Json)) {
          case '\"': Buffer[Size++] = '\"'; break;
          case '\\': Buffer[Size++] = '\\'; break;
          case '/':  Buffer[Size++] = '/'; break;
          case 'b':  Buffer[Size++] = '\b'; break;
          case 'f':  Buffer[Size++] = '\f'; break;
          case 'n':  Buffer[Size++] = '\n'; break;
          case 'r':  Buffer[Size++] = '\r'; break;
          case 't':  Buffer[Size++] = '\t'; break;
          case 'u':  ParseHex(); break;
          default :  goto error;
        }
        break;
      case '\0': goto error;
      default :
        if (JSON_READ8(Json) < ' ')
          goto error;
        Buffer[Size++] = JSON_READ8(Json);
        break;
    }
    JSON_INC(Json);
  }

error:
  FreePool(Buffer);
  return JSON_RETURN_PARSE_ERROR;
}

UINT8 ParseObject(JSON_CONTENT *Json, JSON_VALUE *JsonRoot)
{
  BOOLEAN     IsRoot = TRUE;
  Print(L"Parse Object\n");
  JSON_INC(Json);
  TrimSpaces(Json);
  if (JSON_READ8(Json) == '}') {
    JSON_INC(Json);
    JsonRoot->Type = JsonValueObject;
    return JSON_RETURN_PARSE_OK;
  }
  JSON_VALUE *NewValue = NULL;
  while (TRUE) {
    if (JSON_READ8(Json) != '\"') {
      goto error;
    }
    if (IsRoot) {
      NewValue = JsonRoot;
    } else {
      JSON_INIT_VALUE(NewValue);
      InsertHeadList(&JsonRoot->Item.Link, &NewValue->Item.Link);
    }
    if (ParseStringLiteral(Json, &NewValue->Item.Object.Key.String, &NewValue->Item.Object.Key.Size) != JSON_RETURN_PARSE_OK) {
      goto error;
    }
    TrimSpaces(Json);
    if (JSON_READ8(Json) != ':') {
      goto error;
    }
    JSON_INC(Json);
    NewValue->Item.Object.Value = AllocateZeroPool(sizeof(JSON_VALUE));
    if (ParseValue(Json, NewValue->Item.Object.Value) != 0) {
      goto error;
    }
    
    if (JSON_READ8(Json) == ',') {
      NewValue->Type = JsonValueObject;
      IsRoot = FALSE;
      JSON_INC(Json);
      TrimSpaces(Json);
    } else if (JSON_READ8(Json) == '}') {
      JSON_INC(Json);
      NewValue->Type = JsonValueObject;
      return JSON_RETURN_PARSE_OK;
    } else {
      goto error;
    }
  }

error:
  Print(L"Parse Object error\n");
  if (NewValue->Item.Object.Key.Size != 0) {
    NewValue->Item.Object.Key.Size = 0;
    FreePool(NewValue->Item.Object.Key.String);
    NewValue->Item.Object.Key.String = NULL;
  }
  if (NewValue->Item.Object.Value != NULL) {
    FreePool(NewValue->Item.Object.Value);
    NewValue->Item.Object.Value = NULL;
  }
  if (!IsRoot) {
    RemoveEntryList(&NewValue->Item.Link);
    FreePool(NewValue);
    NewValue = NULL;
  }
  return JSON_RETURN_PARSE_ERROR;
}

UINT8 ParseArray(JSON_CONTENT *Json, JSON_VALUE *JsonRoot)
{
  Print(L"Parse Array\n");
  return 0;
}

UINT8 ParseString(JSON_CONTENT *Json, JSON_VALUE *JsonRoot)
{
  Print(L"Parse String\n");
  if (ParseStringLiteral(Json, &JsonRoot->Item.String.String, &JsonRoot->Item.String.Size) == JSON_RETURN_PARSE_OK) {
    JsonRoot->Type = JsonValueString;
    return JSON_RETURN_PARSE_OK;
  } else 
    return JSON_RETURN_PARSE_ERROR;
}

UINT8 ParseNumber(JSON_CONTENT *Json, JSON_VALUE *JsonRoot)
{
  Print(L"Parse Number\n");
  UINTN Start = Json->Offset;
  if (JSON_READ8(Json) == '-')
    JSON_INC(Json);
  if (JSON_READ8(Json) == '0')
    JSON_INC(Json);
  else {
    if (!IS_1_9(JSON_READ8(Json)))
      return JSON_RETURN_PARSE_ERROR;
    for (JSON_INC(Json); IS_0_9(JSON_READ8(Json)); JSON_INC(Json));
  }
  if (JSON_READ8(Json) == '.') {
    JSON_INC(Json);
    if (!IS_0_9(JSON_READ8(Json)))
      return JSON_RETURN_PARSE_ERROR;
    for (JSON_INC(Json); IS_0_9(JSON_READ8(Json)); JSON_INC(Json));
  }
  if (JSON_READ8(Json) == 'e' || JSON_READ8(Json) == 'E') {
    JSON_INC(Json);
    if (JSON_READ8(Json) == '+' || JSON_READ8(Json) == '-')
      JSON_INC(Json);
    if (!IS_0_9(JSON_READ8(Json)))
      return JSON_RETURN_PARSE_ERROR;
    for (JSON_INC(Json); IS_0_9(JSON_READ8(Json)); JSON_INC(Json));
  }
  // double aaa = strtod(&Json->Buffer[Start], NULL);     //bug
  JsonRoot->Type = JsonValueNumber;
  JsonRoot->Item.String.Size = Json->Offset - Start + 1;
  JsonRoot->Item.String.String = AllocateCopyPool(JsonRoot->Item.String.Size, &Json->Buffer[Start]);
  JsonRoot->Item.String.String[JsonRoot->Item.String.Size - 1] = '\0';
  // AsciiStrToUnicodeStrS(JsonRoot->Item.String.String, Result, 0x100);
  // Print(L"Number:<%s>\n", Result);
  return JSON_RETURN_PARSE_OK;
}

UINT8 ParseLiteral(JSON_CONTENT *Json, JSON_VALUE *JsonRoot, CHAR8 *String, UINT8 Len, JsonValueType Type)
{
  Print(L"Parse Literal\n");
  if (AsciiStrnCmp(&JSON_READ8(Json), String, Len) == 0) {
    JsonRoot->Type = Type;
    JsonRoot->Item.String.Size = Len + 1;
    JsonRoot->Item.String.String = AllocateCopyPool(Len + 1, String);
    Json->Offset += Len;
    return JSON_RETURN_PARSE_OK;
  } else
    return JSON_RETURN_PARSE_ERROR;
}

UINT8 ParseValue(JSON_CONTENT *Json, JSON_VALUE *JsonRoot)
{
  TrimSpaces(Json);
  UINT8 Status = 0;
  switch (JSON_READ8(Json)) {
    case '{':   Status = ParseObject(Json, JsonRoot); break;
    case '[':   Status = ParseArray(Json, JsonRoot); break;
    case '\"':  Status = ParseString(Json, JsonRoot); break;
    default:    Status = ParseNumber(Json, JsonRoot); break;
    case 't':   Status = ParseLiteral(Json, JsonRoot, "true", 4, JsonValueTrue); break;
    case 'f':   Status = ParseLiteral(Json, JsonRoot, "false", 5, JsonValueFalse); break;
    case 'n':   Status = ParseLiteral(Json, JsonRoot, "null", 4, JsonValueNull); break;
  }
  // Print(L"pv offset: %d, last char: %c\n", Json->Offset, Json->Buffer[Json->Offset-1]);     //debug
  TrimSpaces(Json);
  return Status;
}


void ParseJson(CHAR8 *Buffer, UINTN Length)
{
  JSON_CONTENT Json = {Buffer, Length, 0};
  JSON_VALUE *JsonRoot;
  JSON_INIT_VALUE(JsonRoot);
  ParseValue(&Json, JsonRoot);
  Print(L"end offset: %d, last char: <%c>\n", Json.Offset, Json.Buffer[Json.Offset-1]);     //debug
  Print(L"root type:%d;KeySize:%d;value type:%d;Value size:%d;\n",
  JsonRoot->Type, JsonRoot->Item.Object.Key.Size, JsonRoot->Item.Object.Value->Type, JsonRoot->Item.Object.Value->Item.String.Size);
  LIST_ENTRY *Entry = JsonRoot->Item.Link.ForwardLink;
  JSON_VALUE *JsonItem = BASE_CR(Entry, JSON_VALUE, Item.Link);
  Print(L"item type:%d;KeySize:%d;value type:%d;Value size:%d;\n",
  JsonItem->Type, JsonItem->Item.Object.Key.Size, JsonItem->Item.Object.Value->Type, JsonItem->Item.Object.Value->Item.String.Size);
  FreeJson(JsonRoot);
  return;
}

EFI_STATUS
EFIAPI
EntryPoint (
  EFI_HANDLE        ImageHandle,
  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                    Status = EFI_SUCCESS;
  SHELL_FILE_HANDLE             FileHandle;
  UINT64                        Size;
  Status = ShellOpenFileByName(L"test.json", &FileHandle, EFI_FILE_MODE_READ, 0);
  if (Status == EFI_SUCCESS)
    Status = FileHandleGetSize(FileHandle, &Size);
  else
    return Status;

  CHAR8 *Buffer = AllocateZeroPool(Size);
  Status = ShellReadFile(FileHandle, &Size, Buffer);
  Status = ShellCloseFile(&FileHandle);
  ParseJson(Buffer, Size);
  FreePool(Buffer);
  return Status;
}