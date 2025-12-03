#ifndef TYPE_H
#define TYPE_H

#include <wchar.h>

typedef struct _Type {
  wchar_t* type_str;
  struct _Type* array_element_type;
  struct _Type* parent_type;
  int is_array;
} Type;


#endif
