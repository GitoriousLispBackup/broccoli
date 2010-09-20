#ifndef _H_constant

#define _H_constant

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

#define EXACTLY       0
#define AT_LEAST      1
#define NO_MORE_THAN  2
#define RANGE         3

#define OFF           0
#define ON            1
#define LHS           0
#define RHS           1
#define NESTED_RHS    2
#define NEGATIVE      0
#define POSITIVE      1
#define EOS           '\0'

#define INSIDE        0
#define OUTSIDE       1

#define LESS_THAN     0
#define GREATER_THAN  1
#define EQUAL         2

#define GLOBAL_SAVE   0
#define LOCAL_SAVE    1
#define VISIBLE_SAVE  2

#ifndef WPROMPT_STRING
#define WPROMPT_STRING "wbroccoli"
#endif

#ifndef APPLICATION_NAME
#define APPLICATION_NAME "Broccoli"
#endif

#ifndef COMMAND_PROMPT
#define COMMAND_PROMPT "broccoli> "
#endif

#ifndef VERSION_STRING
#define VERSION_STRING "0.1.4 (abominable)"
#endif

#ifndef EMAIL_STRING
#define EMAIL_STRING    "broccoli@earthvssoup.com"
#endif

#ifndef CREATION_DATE_STRING
#define CREATION_DATE_STRING "2008.02.05"
#endif

#ifndef BANNER_STRING
#define BANNER_STRING "-= Broccoli v0.1.4 (abominable) by Fogus =-\nhttp://projects.earthvssoup.com/broccoli\n"
#endif

/************************
 * TOKEN AND TYPE VALUES
 *************************/

#define OBJECT_TYPE_NAME            "OBJECT"
#define USER_TYPE_NAME              "USER"
#define PRIMITIVE_TYPE_NAME         "PRIMITIVE"
#define NUMBER_TYPE_NAME            "NUMBER"
#define INTEGER_TYPE_NAME           "INTEGER"
#define FLOAT_TYPE_NAME             "FLOAT"
#define ATOM_TYPE_NAME              "ATOM"
#define STRING_TYPE_NAME            "STRING"
#define LIST_TYPE_NAME              "LIST"
/* Lexemes are sequences of characters that make up logical units.  Atoms and strings.*/
#define LEXEME_TYPE_NAME            "LEXEME"
#define ADDRESS_TYPE_NAME           "ADDRESS"
#define EXTERNAL_ADDRESS_TYPE_NAME  "EXTERNAL-ADDRESS"
#define INSTANCE_TYPE_NAME          "INSTANCE"
#define INSTANCE_NAME_TYPE_NAME     "INSTANCE-NAME"
#define INSTANCE_ADDRESS_TYPE_NAME  "INSTANCE-ADDRESS"

/************************************************************************
 * The values of these constants should not be changed.  They are set to
 * start after the primitive type codes in CONSTANT.H.  These codes are
 * used to let the generic function bsave image be used whether COOL is
 * present or not.
 *************************************************************************/

#define OBJECT_TYPE_CODE                9
#define PRIMITIVE_TYPE_CODE             10
#define NUMBER_TYPE_CODE                11
#define LEXEME_TYPE_CODE                12
#define ADDRESS_TYPE_CODE               13
#define INSTANCE_TYPE_CODE              14

/***************************************************
 * The first 9 primitive types need to retain their
 * values!! Sorted arrays depend on their values!!
 ****************************************************/

#define FLOAT                           0
#define INTEGER                         1
#define ATOM                            2
#define STRING                          3
#define LIST                            4
#define EXTERNAL_ADDRESS                5
#define UNUSED_1                        6
#define INSTANCE_ADDRESS                7
#define INSTANCE_NAME                   8

#define FCALL                          30
#define GCALL                          31
#define PCALL                          32

#define SCALAR_VARIABLE                35
#define LIST_VARIABLE                  36
#define UNUSED_2                       37
#define UNUSED_3                       38
#define BITMAPARRAY                    39
#define DATA_OBJECT_ARRAY              40

#define OBJ_GET_SLOT_PNVAR1            70
#define OBJ_GET_SLOT_PNVAR2            71
#define OBJ_GET_SLOT_JNVAR1            72
#define OBJ_GET_SLOT_JNVAR2            73
#define OBJ_SLOT_LENGTH                74
#define OBJ_PN_CONSTANT                75
#define OBJ_PN_CMP1                    76
#define OBJ_JN_CMP1                    77
#define OBJ_PN_CMP2                    78
#define OBJ_JN_CMP2                    79
#define OBJ_PN_CMP3                    80
#define OBJ_JN_CMP3                    81
#define DEFCLASS_PTR                   82
#define HANDLER_GET                    83
#define HANDLER_PUT                    84

#define FUNCTION_ARG                   95
#define FUNCTION_OPTIONAL_ARG          96
#define FUNCTION_VAR_META              97
#define FUNCTION_VAR                   98

#define NOT_CONSTRAINT                160
#define AND_CONSTRAINT                161
#define OR_CONSTRAINT                 162
#define PREDICATE_CONSTRAINT          163
#define RETURN_VALUE_CONSTRAINT       164

#define LPAREN                        170
#define RPAREN                        171
#define STOP                          172
#define UNKNOWN_VALUE                 173

#define RVOID                         175

#define INTEGER_OR_FLOAT              180
#define ATOM_OR_STRING                181
#define INSTANCE_OR_INSTANCE_NAME     182

/***
 * Return type definitions
 **/
#define RT_EXT_ADDRESS                                  'a'
#define RT_BOOL                                         'b'
#define RT_CHAR                                         'c'
#define RT_DOUBLE                                       'd'
#define RT_INSTANCE_NAME                                'e'
#define RT_FLOAT                                        'f'
#define RT_LONG_LONG                                    'g'
#define RT_UNUSED_0                                     'h'
#define RT_INT                                          'i'
#define RT_ATOM_STRING_INST                             'j'
#define RT_ATOM_STRING                                  'k'
#define RT_LONG                                         'l'
#define RT_LIST                                         'm'
#define RT_INT_FLOAT                                    'n'
#define RT_INSTANCE                                     'o'
#define RT_INSTANCE_NAME_ATOM                           'p'
#define RT_LIST_ATOM_STRING                             'q'
#define RT_UNUSED_1                                     'r'
#define RT_STRING                                       's'
#define RT_UNUSED_2                                     't'
#define RT_UNKNOWN                                      'u'
#define RT_VOID                                         'v'
#define RT_ATOM                                         'w'
#define RT_INST_ADDRESS                                 'x'
#define RT_UNUSED_3                                     'y'
#define RT_UNUSED_4                                     'z'

/***
 * Assignment name and constraints
 **/

#define FUNC_NAME_ASSIGNMENT    ":="
#define FUNC_CNSTR_ASSIGNMENT   NULL

#define FUNC_NAME_CREATE_LIST   "list"
#define FUNC_CNSTR_CREATE_LIST  NULL

#define FUNC_NAME_CREATE_FUNC   "fn"
#define FUNC_CNSTR_CREATE_FUNC  NULL

#define FUNC_NAME_PROGN         "(progn)"
#define FUNC_CNSTR_PROGN        NULL

#define FUNC_NAME_EXPAND        "(expand)"
#define FUNC_CNSTR_EXPAND       "11m"

#define FUNC_NAME_EXPAND_META   "(expansion-call)"
#define FUNC_CNSTR_EXPAND_META  NULL


/***
 * Output fields and constants
 **/
#define OUT_CRLF                "endl"
#define OUT_TAB                 "tab"

#define TYPE_LIST_NAME          "list"
#define TYPE_FLOAT_NAME         "float"
#define TYPE_INT_NAME           "integer"
#define TYPE_POINTER_NAME       "pointer"
#define TYPE_UNDEFINED          "undefined"

#endif
