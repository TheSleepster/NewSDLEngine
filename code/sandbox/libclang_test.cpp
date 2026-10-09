/* ========================================================================
   $File: libclang_test.cpp $
   $Date: October 06 2026 12:47 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#include <stdio.h>
#include <clang-c/Index.h>

struct test_structure_t
{
    int   test0;
    float test1;
    int   test2;

    test_structure_t *next_structure;
};

int
main(void)
{
    CXIndex index = clang_createIndex(0, 1); 
    CXTranslationUnit translation_unit = clang_parseTranslationUnit(index, 
                                                                    "../code/sandbox/libclang_test.cpp",
                                                                    null,
                                                                    0,
                                                                    null,
                                                                    0,
                                                                    CXTranslationUnit_None);
    CXCursor translation_unit_cursor = clang_getTranslationUnitCursor(translation_unit);
    (void)translation_unit_cursor;

    clang_visitChildren(translation_unit_cursor, 
    [](CXCursor current_cursor, CXCursor parent, CXClientData client_data)
    {
        (void)parent;
        (void)client_data;
        CXCursorKind AST_kind = clang_getCursorKind(current_cursor); 
        if(AST_kind != CXCursor_StructDecl &&
           AST_kind != CXCursor_FieldDecl)
        {
            return(CXChildVisit_Recurse);
        }

        CXType element_type = clang_getCursorType(current_cursor);

        CXString element_name      = clang_getCursorDisplayName(current_cursor);
        CXString element_type_name = clang_getTypeSpelling(element_type);

        printf("Element name: %s\n", (char*)element_name.data);
        printf("Element typename: %s\n", (char*)element_type_name.data);

        clang_disposeString(element_name);
        clang_disposeString(element_type_name);

        return(CXChildVisit_Recurse);
    },
    null);

    return(0);
}
