/* Purpose: Routines for scanning lexical tokens from an
 *   input source.                                           */

#define __CORE_SCANNER_SOURCE__

#include <ctype.h>
#include <stdio.h>
#define _STDIO_INCLUDED_
#include <string.h>
#include <limits.h>

#include "setup.h"
#include "constant.h"
#include "core_environment.h"
#include "router.h"
#include "type_symbol.h"
#include "core_gc.h"
#include "core_memory.h"
#include "sysdep.h"

#include "core_scanner.h"

#include <stdlib.h>

/**************************************
 * LOCAL FUNCTION PROTOTYPES
 ***************************************/

static void                   * _scan_atom(void *, char *, int, unsigned short *);
static void                   * _scan_string(void *, char *);
static void                     _scan_number(void *, char *, struct token *);
static void                     _delete_scanner_data(void *);

/***********************************************
 * core_init_scanner_data: Allocates environment
 *    data for scanner routines.
 ************************************************/
void core_init_scanner_data(void *env)
{
    core_allocate_environment_data(env, SCANNER_DATA_INDEX, sizeof(struct core_scanner_data), _delete_scanner_data);
}

/*************************************************
 * DeallocateScannerData: Deallocates environment
 *    data for scanner routines.
 **************************************************/
static void _delete_scanner_data(void *env)
{
    if( core_get_scanner_data(env)->global_max !=  0 )
    {
        core_mem_free(env, core_get_scanner_data(env)->global_str, core_get_scanner_data(env)->global_max);
    }
}

/**********************************************************************
 * core_get_token: Reads next token from the input stream. The pointer to
 *   the token data structure passed as an argument is set to contain
 *   the type of token (e.g., symbol, string, integer, etc.), the data
 *   value for the token (i.e., a symbol table location if it is a
 *   symbol or string, an integer table location if it is an integer),
 *   and the pretty print representation.
 ***********************************************************************/
void core_get_token(void *env, char *logicalName, struct token *theToken)
{
    int inchar;
    unsigned short type;

    /*=======================================
     * Set Unknown default values for token.
     *=======================================*/

    theToken->type = UNKNOWN_VALUE;
    theToken->value = NULL;
    theToken->pp = "unknown";
    core_get_scanner_data(env)->global_position = 0;
    core_get_scanner_data(env)->global_max = 0;

    /*==============================================
     * Remove all white space before processing the
     * core_get_token() request.
     *==============================================*/

    inchar = get_ch_router(env, logicalName);

    while((inchar == ' ') || (inchar == '\n') || (inchar == '\f') ||
          (inchar == '\r') || (inchar == ';') || (inchar == '\t'))
    {
        /*=======================
         * Remove comment lines.
         *=======================*/

        if( inchar == ';' )
        {
            inchar = get_ch_router(env, logicalName);

            while((inchar != '\n') && (inchar != '\r') && (inchar != EOF) )
            {
                inchar = get_ch_router(env, logicalName);
            }
        }

        inchar = get_ch_router(env, logicalName);
    }

    /*==========================
     * Process Symbolic Tokens.
     *==========================*/

    if( isalpha(inchar))
    {
        theToken->type = ATOM;
        unget_ch_router(env, inchar, logicalName);
        theToken->value = (void *)_scan_atom(env, logicalName, 0, &type);
        theToken->pp = to_string(theToken->value);
    }

    /*===============================================
     * Process Number Tokens beginning with a digit.
     *===============================================*/

    else if( isdigit(inchar))
    {
        unget_ch_router(env, inchar, logicalName);
        _scan_number(env, logicalName, theToken);
    }

    else
    {
        switch( inchar )
        {
            /*========================
             * Process String Tokens.
             *========================*/

        case '"':
            theToken->value = (void *)_scan_string(env, logicalName);
            theToken->type = STRING;
            theToken->pp = core_gc_string_printform(env, to_string(theToken->value));
            break;

            /*=======================================
             * Process Tokens that might be numbers.
             *=======================================*/

        case '-':
        case '.':
        case '+':
            unget_ch_router(env, inchar, logicalName);
            _scan_number(env, logicalName, theToken);
            break;

            /*===================================
             * Process ? and ?<variable> Tokens.
             *===================================*/

        case '$':
            inchar = get_ch_router(env, logicalName);

            if( isalpha(inchar))
            {
                unget_ch_router(env, inchar, logicalName);
                unget_ch_router(env, '$', logicalName);
                theToken->value = (void *)_scan_atom(env, logicalName, 0, &type);
                theToken->type = SCALAR_VARIABLE;
                theToken->pp = core_gc_append_strings(env, "", to_string(theToken->value));
            }

            break;

        case '@':
            inchar = get_ch_router(env, logicalName);

            if( isalpha(inchar))
            {
                unget_ch_router(env, inchar, logicalName);
                unget_ch_router(env, '@', logicalName);
                theToken->value = (void *)_scan_atom(env, logicalName, 0, &type);
                theToken->type = LIST_VARIABLE;
                theToken->pp = core_gc_append_strings(env, "", to_string(theToken->value));
            }

            break;

            /*=============================================
             * Process "(", ")", "~", "|", and "&" Tokens.
             *=============================================*/

        case '(':
            theToken->type = LPAREN;
            theToken->value = (void *)store_atom(env, "(");
            theToken->pp = "(";
            break;

        case ')':
            theToken->type = RPAREN;
            theToken->value = (void *)store_atom(env, ")");
            theToken->pp = ")";
            break;

        case '~':
            theToken->type = NOT_CONSTRAINT;
            theToken->value = (void *)store_atom(env, "~");
            theToken->pp = "~";
            break;

        case '|':
            theToken->type = OR_CONSTRAINT;
            theToken->value = (void *)store_atom(env, "|");
            theToken->pp = "|";
            break;

        case '&':
            theToken->type =  AND_CONSTRAINT;
            theToken->value = (void *)store_atom(env, "&");
            theToken->pp = "&";
            break;

            /*============================
             * Process End-of-File Token.
             *============================*/

        case EOF:
        case 0:
        case 3:
            theToken->type = STOP;
            theToken->value = (void *)store_atom(env, "stop");
            theToken->pp = "";
            break;

            /*=======================
             * Process Other Tokens.
             *=======================*/

        default:

            if( isprint(inchar))
            {
                unget_ch_router(env, inchar, logicalName);
                theToken->value = (void *)_scan_atom(env, logicalName, 0, &type);
                theToken->type = type;
                theToken->pp = to_string(theToken->value);
            }
            else
            {
                theToken->pp = "<<<unprintable character>>>";
            }

            break;
        }
    }

    /*===============================================
     * Put the new token in the pretty print buffer.
     *===============================================*/


    if( theToken->type == INSTANCE_NAME )
    {
        core_save_pp_buffer(env, "[");
        core_save_pp_buffer(env, theToken->pp);
        core_save_pp_buffer(env, "]");
    }
    else
    {
        core_save_pp_buffer(env, theToken->pp);
    }


    /*=========================================================
     * Return the temporary memory used in scanning the token.
     *=========================================================*/

    if( core_get_scanner_data(env)->global_str != NULL )
    {
        core_mem_release(env, core_get_scanner_data(env)->global_str, core_get_scanner_data(env)->global_max);
        core_get_scanner_data(env)->global_str = NULL;
        core_get_scanner_data(env)->global_max = 0;
        core_get_scanner_data(env)->global_position = 0;
    }

    return;
}

/************************************
 * ScanSymbol: Scans a symbol token.
 *************************************/
static void *_scan_atom(void *env, char *logicalName, int count, unsigned short *type)
{
    int inchar;

#if OBJECT_SYSTEM
    void *symbol;
#endif

    /*=====================================
     * Scan characters and add them to the
     * symbol until a delimiter is found.
     *=====================================*/

    inchar = get_ch_router(env, logicalName);

    while( (inchar != '"') &&
           (inchar != '(') && (inchar != ')') &&
           (inchar != '&') && (inchar != '|') && (inchar != '~') &&
           (inchar != ' ') && (inchar != ';') &&
           isprint(inchar) )
    {
        core_get_scanner_data(env)->global_str = core_gc_expand_string(env, inchar, core_get_scanner_data(env)->global_str, &core_get_scanner_data(env)->global_position, &core_get_scanner_data(env)->global_max, core_get_scanner_data(env)->global_max + 80);

        count++;
        inchar = get_ch_router(env, logicalName);
    }

    /*===================================================
     * Return the last character scanned (the delimiter)
     * to the input stream so it will be scanned as part
     * of the next token.
     *===================================================*/

    unget_ch_router(env, inchar, logicalName);

    /*====================================================
     * Add the symbol to the symbol table and return the
     * symbol table address of the symbol. Symbols of the
     * form [<symbol>] are instance names, so the type
     * returned is INSTANCE_NAME rather than ATOM.
     *====================================================*/

#if OBJECT_SYSTEM

    if( count > 2 )
    {
        if((core_get_scanner_data(env)->global_str[0] == '[') ? (core_get_scanner_data(env)->global_str[count - 1] == ']') : FALSE )
        {
            *type = INSTANCE_NAME;
            inchar = ']';
        }
        else
        {
            *type = ATOM;
            return(store_atom(env, core_get_scanner_data(env)->global_str));
        }

        core_get_scanner_data(env)->global_str[count - 1] = EOS;
        symbol = store_atom(env, core_get_scanner_data(env)->global_str + 1);
        core_get_scanner_data(env)->global_str[count - 1] = (char)inchar;
        return(symbol);
    }
    else
    {
        *type = ATOM;
        return(store_atom(env, core_get_scanner_data(env)->global_str));
    }

#else
    *type = ATOM;
    return(store_atom(env, core_get_scanner_data(env)->global_str));

#endif
}

/************************************
 * ScanString: Scans a string token.
 *************************************/
static void *_scan_string(void *env, char *logicalName)
{
    int inchar;
    size_t pos = 0;
    size_t max = 0;
    char *theString = NULL;
    void *thePtr;

    /*============================================
     * Scan characters and add them to the string
     * until the " delimiter is found.
     *============================================*/

    inchar = get_ch_router(env, logicalName);

    while((inchar != '"') && (inchar != EOF))
    {
        if( inchar == '\\' )
        {
            inchar = get_ch_router(env, logicalName);
        }

        theString = core_gc_expand_string(env, inchar, theString, &pos, &max, max + 80);
        inchar = get_ch_router(env, logicalName);
    }

    if((inchar == EOF) && (core_get_scanner_data(env)->ignoring_completion_error == FALSE))
    {
        error_print_id(env, "SCANNER", 1, TRUE);
        print_router(env, WERROR, "Encountered End-Of-File while scanning a string\n");
    }

    /*===============================================
     * Add the string to the symbol table and return
     * the symbol table address of the string.
     *===============================================*/

    if( theString == NULL )
    {
        thePtr = store_atom(env, "");
    }
    else
    {
        thePtr = store_atom(env, theString);
        core_mem_release(env, theString, max);
    }

    return(thePtr);
}

/*************************************
 * ScanNumber: Scans a numeric token.
 **************************************/
static void _scan_number(void *env, char *logicalName, struct token *theToken)
{
    int count = 0;
    int inchar, phase;
    int digitFound = FALSE;
    int processFloat = FALSE;
    double fvalue;
    long long lvalue;
    unsigned short type;

    /* Phases:
     *  -1 = sign
     *   0 = integral
     *   1 = decimal
     *   2 = exponent-begin
     *   3 = exponent-value
     *   5 = done
     *   9 = error          */

    inchar = get_ch_router(env, logicalName);
    phase = -1;

    while((phase != 5) && (phase != 9))
    {
        if( phase == -1 )
        {
            if( isdigit(inchar))
            {
                phase = 0;
                digitFound = TRUE;
                core_get_scanner_data(env)->global_str = core_gc_expand_string(env, inchar, core_get_scanner_data(env)->global_str, &core_get_scanner_data(env)->global_position, &core_get_scanner_data(env)->global_max, core_get_scanner_data(env)->global_max + 80);
                count++;
            }
            else if((inchar == '+') || (inchar == '-'))
            {
                phase = 0;
                core_get_scanner_data(env)->global_str = core_gc_expand_string(env, inchar, core_get_scanner_data(env)->global_str, &core_get_scanner_data(env)->global_position, &core_get_scanner_data(env)->global_max, core_get_scanner_data(env)->global_max + 80);
                count++;
            }
            else if( inchar == '.' )
            {
                processFloat = TRUE;
                core_get_scanner_data(env)->global_str = core_gc_expand_string(env, inchar, core_get_scanner_data(env)->global_str, &core_get_scanner_data(env)->global_position, &core_get_scanner_data(env)->global_max, core_get_scanner_data(env)->global_max + 80);
                count++;
                phase = 1;
            }
            else if((inchar == 'E') || (inchar == 'e'))
            {
                processFloat = TRUE;
                core_get_scanner_data(env)->global_str = core_gc_expand_string(env, inchar, core_get_scanner_data(env)->global_str, &core_get_scanner_data(env)->global_position, &core_get_scanner_data(env)->global_max, core_get_scanner_data(env)->global_max + 80);
                count++;
                phase = 2;
            }
            else if( (inchar == '<') || (inchar == '"') ||
                     (inchar == '(') || (inchar == ')') ||
                     (inchar == '&') || (inchar == '|') || (inchar == '~') ||
                     (inchar == ' ') || (inchar == ';') ||
                     (isprint(inchar) == 0) )
            {
                phase = 5;
            }
            else
            {
                phase = 9;
                core_get_scanner_data(env)->global_str = core_gc_expand_string(env, inchar, core_get_scanner_data(env)->global_str, &core_get_scanner_data(env)->global_position, &core_get_scanner_data(env)->global_max, core_get_scanner_data(env)->global_max + 80);
                count++;
            }
        }
        else if( phase == 0 )
        {
            if( isdigit(inchar))
            {
                digitFound = TRUE;
                core_get_scanner_data(env)->global_str = core_gc_expand_string(env, inchar, core_get_scanner_data(env)->global_str, &core_get_scanner_data(env)->global_position, &core_get_scanner_data(env)->global_max, core_get_scanner_data(env)->global_max + 80);
                count++;
            }
            else if( inchar == '.' )
            {
                processFloat = TRUE;
                core_get_scanner_data(env)->global_str = core_gc_expand_string(env, inchar, core_get_scanner_data(env)->global_str, &core_get_scanner_data(env)->global_position, &core_get_scanner_data(env)->global_max, core_get_scanner_data(env)->global_max + 80);
                count++;
                phase = 1;
            }
            else if((inchar == 'E') || (inchar == 'e'))
            {
                processFloat = TRUE;
                core_get_scanner_data(env)->global_str = core_gc_expand_string(env, inchar, core_get_scanner_data(env)->global_str, &core_get_scanner_data(env)->global_position, &core_get_scanner_data(env)->global_max, core_get_scanner_data(env)->global_max + 80);
                count++;
                phase = 2;
            }
            else if( (inchar == '<') || (inchar == '"') ||
                     (inchar == '(') || (inchar == ')') ||
                     (inchar == '&') || (inchar == '|') || (inchar == '~') ||
                     (inchar == ' ') || (inchar == ';') ||
                     (isprint(inchar) == 0) )
            {
                phase = 5;
            }
            else
            {
                phase = 9;
                core_get_scanner_data(env)->global_str = core_gc_expand_string(env, inchar, core_get_scanner_data(env)->global_str, &core_get_scanner_data(env)->global_position, &core_get_scanner_data(env)->global_max, core_get_scanner_data(env)->global_max + 80);
                count++;
            }
        }
        else if( phase == 1 )
        {
            if( isdigit(inchar))
            {
                digitFound = TRUE;
                core_get_scanner_data(env)->global_str = core_gc_expand_string(env, inchar, core_get_scanner_data(env)->global_str, &core_get_scanner_data(env)->global_position, &core_get_scanner_data(env)->global_max, core_get_scanner_data(env)->global_max + 80);
                count++;
            }
            else if((inchar == 'E') || (inchar == 'e'))
            {
                core_get_scanner_data(env)->global_str = core_gc_expand_string(env, inchar, core_get_scanner_data(env)->global_str, &core_get_scanner_data(env)->global_position, &core_get_scanner_data(env)->global_max, core_get_scanner_data(env)->global_max + 80);
                count++;
                phase = 2;
            }
            else if( (inchar == '<') || (inchar == '"') ||
                     (inchar == '(') || (inchar == ')') ||
                     (inchar == '&') || (inchar == '|') || (inchar == '~') ||
                     (inchar == ' ') || (inchar == ';') ||
                     (isprint(inchar) == 0) )
            {
                phase = 5;
            }
            else
            {
                phase = 9;
                core_get_scanner_data(env)->global_str = core_gc_expand_string(env, inchar, core_get_scanner_data(env)->global_str, &core_get_scanner_data(env)->global_position, &core_get_scanner_data(env)->global_max, core_get_scanner_data(env)->global_max + 80);
                count++;
            }
        }
        else if( phase == 2 )
        {
            if( isdigit(inchar))
            {
                core_get_scanner_data(env)->global_str = core_gc_expand_string(env, inchar, core_get_scanner_data(env)->global_str, &core_get_scanner_data(env)->global_position, &core_get_scanner_data(env)->global_max, core_get_scanner_data(env)->global_max + 80);
                count++;
                phase = 3;
            }
            else if((inchar == '+') || (inchar == '-'))
            {
                core_get_scanner_data(env)->global_str = core_gc_expand_string(env, inchar, core_get_scanner_data(env)->global_str, &core_get_scanner_data(env)->global_position, &core_get_scanner_data(env)->global_max, core_get_scanner_data(env)->global_max + 80);
                count++;
                phase = 3;
            }
            else if( (inchar == '<') || (inchar == '"') ||
                     (inchar == '(') || (inchar == ')') ||
                     (inchar == '&') || (inchar == '|') || (inchar == '~') ||
                     (inchar == ' ') || (inchar == ';') ||
                     (isprint(inchar) == 0) )
            {
                digitFound = FALSE;
                phase = 5;
            }
            else
            {
                phase = 9;
                core_get_scanner_data(env)->global_str = core_gc_expand_string(env, inchar, core_get_scanner_data(env)->global_str, &core_get_scanner_data(env)->global_position, &core_get_scanner_data(env)->global_max, core_get_scanner_data(env)->global_max + 80);
                count++;
            }
        }
        else if( phase == 3 )
        {
            if( isdigit(inchar))
            {
                core_get_scanner_data(env)->global_str = core_gc_expand_string(env, inchar, core_get_scanner_data(env)->global_str, &core_get_scanner_data(env)->global_position, &core_get_scanner_data(env)->global_max, core_get_scanner_data(env)->global_max + 80);
                count++;
            }
            else if( (inchar == '<') || (inchar == '"') ||
                     (inchar == '(') || (inchar == ')') ||
                     (inchar == '&') || (inchar == '|') || (inchar == '~') ||
                     (inchar == ' ') || (inchar == ';') ||
                     (isprint(inchar) == 0) )
            {
                if((core_get_scanner_data(env)->global_str[count - 1] == '+') || (core_get_scanner_data(env)->global_str[count - 1] == '-'))
                {
                    digitFound = FALSE;
                }

                phase = 5;
            }
            else
            {
                phase = 9;
                core_get_scanner_data(env)->global_str = core_gc_expand_string(env, inchar, core_get_scanner_data(env)->global_str, &core_get_scanner_data(env)->global_position, &core_get_scanner_data(env)->global_max, core_get_scanner_data(env)->global_max + 80);
                count++;
            }
        }

        if((phase != 5) && (phase != 9))
        {
            inchar = get_ch_router(env, logicalName);
        }
    }

    if( phase == 9 )
    {
        theToken->value = (void *)_scan_atom(env, logicalName, count, &type);
        theToken->type = type;
        theToken->pp = to_string(theToken->value);
        return;
    }

    /*=======================================
     * Stuff last character back into buffer
     * and return the number.
     *=======================================*/

    unget_ch_router(env, inchar, logicalName);

    if( !digitFound )
    {
        theToken->type = ATOM;
        theToken->value = (void *)store_atom(env, core_get_scanner_data(env)->global_str);
        theToken->pp = to_string(theToken->value);
        return;
    }

    if( processFloat )
    {
        fvalue = atof(core_get_scanner_data(env)->global_str);
        theToken->type = FLOAT;
        theToken->value = (void *)store_double(env, fvalue);
        theToken->pp = core_convert_float_to_string(env, to_double(theToken->value));
    }
    else
    {
#if WIN_MVC
        lvalue = _strtoi64(core_get_scanner_data(env)->global_str, NULL, 10);
#else
        lvalue = strtoll(core_get_scanner_data(env)->global_str, NULL, 10);
#endif

        if((lvalue == LLONG_MAX) || (lvalue == LLONG_MIN))
        {
            warning_print_id(env, "SCANNER", 1, FALSE);
            print_router(env, WWARNING, "Over or underflow of long long integer.\n");
        }

        theToken->type = INTEGER;
        theToken->value = (void *)store_long(env, lvalue);
        theToken->pp = core_conver_long_to_string(env, to_long(theToken->value));
    }

    return;
}

/**********************************************************
 * core_copy_token: Copies values of one token to another token.
 ***********************************************************/
void core_copy_token(struct token *destination, struct token *source)
{
    destination->type = source->type;
    destination->value = source->value;
    destination->pp = source->pp;
}

/***************************************
 * core_reset_line_count: Resets the scanner's
 *   line count to zero.
 ****************************************/
void core_reset_line_count(void *env)
{
    core_get_scanner_data(env)->line_count = 0;
}

/***************************************************
 * GettLineCount: Returns the scanner's line count.
 ****************************************************/
long core_get_line_count(void *env)
{
    return(core_get_scanner_data(env)->line_count);
}

/*********************************
 * core_inc_line_count: Increments
 *   the scanner's line count.
 **********************************/
void core_inc_line_count(void *env)
{
    core_get_scanner_data(env)->line_count++;
}

/*********************************
 * core_dec_line_count: Decrements
 *   the scanner's line count.
 **********************************/
void core_dec_line_count(void *env)
{
    core_get_scanner_data(env)->line_count--;
}
