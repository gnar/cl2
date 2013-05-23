/*
    This file is part of the CL2 script language interpreter.

    Gunnar Selke <gunnar@gmx.info>

    CL2 is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    CL2 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with CL2; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include "compiler/cllexer.h"

#include <assert.h>

CLLexeme CLLexer::error(const char *reason)
{
	CLLexeme lexeme(TOK_ERROR);
	lexeme.str = reason;
	err = true;
	return lexeme;
}

CLLexer::CLLexer(std::istream &inp, const std::string &filename) : input(inp), filename(filename)
{
	lineno = 1;
	eof = false; err = false;
	ch = -1;

	next();
}

CLLexeme CLLexer::lex()
{
	char tmp;

	if (err) return error("<error>");

	while (!eof)
	{
		switch (ch)
		{
			case '\n': // eat whitespace and line feed
				++lineno;
			case '\t':
			case '\r':
			case ' ': next(); continue;

			case '+': next(); return('+');
			case '-': next(); return('-');
			case '*': next(); return('*');
			case '/': // divide or comment
				next();
				if (ch == '/' || ch == '*')
				{
					if (!eatComment()) return(error("Comment error")); 
					continue;
				}
				return('/');

			case '=': // = or == 
				next();
				if (ch == '=') { next(); return(TOK_EQ); }
				return('=');

			case '!': // ! or !=
				next();
				if (ch == '=') { next(); return(TOK_NEQ); }
				return('!');

			case '<': // < or <= or <<
				next();
				if (ch == '=') { next(); return(TOK_LE); }
				if (ch == '<') { next(); return(TOK_SHL); }
				return('<');

			case '>': // > or >= or >>
				next();
				if (ch == '=') { next(); return(TOK_GE); }
				if (ch == '>') { next(); return(TOK_SHR); }
				return('>');

			case ':': // ::
				next();
				if (ch == ':') { next(); return(TOK_DOUBLECOLON); }
				return(':');

			case '$': case '@':
			case '.': case ',':
			case '^':
			case '|':
			case '&':
			case '%':
			case ';':
			case '{': case '}':
			case '[': case ']':
			case '(': case ')':
				tmp = ch; 
				next(); 
				return(tmp);

			case '"': // begin string
				return(readString());

			case '#':
				// ignore first line beginning with #
				if (lineno == 1)
				{
					while (!eof && (ch != '\n'))
					{
						next();
					}
					return(lex());
				}
				return error("Unexpected character");

			default:
				if (isdigit(ch))
				{
					return(readNumber());
				} else if (isalpha(ch) || ch == '_') {
					return(readKeywordOrIdentifier());
				}
				// Error!
				return(error("Unexpected character"));
				break;
		}
	}

	return(TOK_EOF);
}

void CLLexer::next()
{
	// TODO: Buffering... ?
	input.read(&ch, sizeof(char));

	if (input.eof()) eof = true; 
}

CLLexeme CLLexer::readNumber()
{
	bool isfloat = false;
	int bufpos = 0;
	
	while ((isdigit(ch) || ch == '.') && !eof)
	{
		if (ch == '.') isfloat = true;	
		tmpstr[bufpos++] = ch;
		next();
	}
	tmpstr[bufpos++] = '\0';
	
	CLLexeme lexeme(isfloat ? TOK_FLOAT : TOK_INTEGER);
	if (isfloat) lexeme.real = std::strtod(tmpstr, NULL); else lexeme.integer = std::atoi(tmpstr);
	return lexeme;
}

CLLexeme CLLexer::readString()
{
	int bufpos = 0;
	next(); // skip "

	while (ch != '"' && !eof)
	{
		switch (ch)
		{
			case '\n': return error("Newline inside string");
			case '\\': // escape character
				next(); if (eof) return error("End of file inside string");
				switch (ch)
				{
					case '\n': tmpstr[bufpos++] = '\n'; break; // escaped line break
					case '\\': tmpstr[bufpos++] = '\\'; break;
					case '"': tmpstr[bufpos++] = '"'; break;
					case 't': tmpstr[bufpos++] = '\t'; break;
					case 'a': tmpstr[bufpos++] = '\a'; break;
					case 'b': tmpstr[bufpos++] = '\b'; break;
					case 'n': tmpstr[bufpos++] = '\n'; break;
					case 'r': tmpstr[bufpos++] = '\r'; break;
					case 'v': tmpstr[bufpos++] = '\v'; break;
					case 'f': tmpstr[bufpos++] = '\f'; break;
					default: return error("Unrecognized escape character"); 
				}
				break;
			default: tmpstr[bufpos++] = ch; break;
		}
		next();
	}
	tmpstr[bufpos++] = '\0';

	if (eof) return error("End of file inside string");
	next(); // skip ending "

	CLLexeme lexeme(TOK_STRING);
	lexeme.str = tmpstr;
	return lexeme;
}

static struct CLKeywordDef
{
	char *name;
	CLToken tok;
} keywords[] = {
	{"if", TOK_IF},
	{"else", TOK_ELSE},
	{"while", TOK_WHILE},
	{"do", TOK_DO},
	{"and", TOK_AND},
	{"not", TOK_NOT},
	{"or", TOK_OR},
	{"break", TOK_BREAK},
	{"local", TOK_LOCAL},
	{"null", TOK_NULL},
	{"true", TOK_TRUE},
	{"false", TOK_FALSE},
	{"function", TOK_FUNCTION},
	{"return", TOK_RETURN},
	{"root", TOK_ROOT},
	{"self", TOK_SELF},
	{"yield", TOK_YIELD},
	{"break", TOK_BREAK},
	{"for", TOK_FOR},
	{"external", TOK_EXTERNAL},
	{"foreach", TOK_FOREACH},
	{"in", TOK_IN},
	{"array", TOK_ARRAY},
	{"table", TOK_TABLE},
	{"clone", TOK_CLONE},
	{"switch", TOK_SWITCH},
	{"case", TOK_CASE},
#ifdef MINDBENDER_EXT
	{"event", TOK_EVENT},
#endif
};
static int num_keywords = sizeof(keywords) / sizeof(CLKeywordDef);

CLLexeme CLLexer::readKeywordOrIdentifier()
{
	int bufpos = 0;

	while ((isalnum(ch) || ch == '_') && !eof)
	{
		tmpstr[bufpos++] = ch;
		next();
	}
	tmpstr[bufpos++] = '\0';

	// check if buffer is a keyword..
	for (int i=0; i<num_keywords; ++i)
	{
		if (std::strcmp(keywords[i].name, tmpstr) == 0) return CLLexeme(keywords[i].tok);
	}

	// buffer is a identifier
	CLLexeme lexeme(TOK_IDENTIFIER);
	lexeme.str = tmpstr;
	return lexeme;
}

bool CLLexer::eatComment()
{
	if (ch == '/') // C++ style comment
	{
		char pr = -1;
		do
		{
			pr = ch;
			next();

			// parse nested comments
			if (pr == '/' && ch == '*')
			{
				if (!eatComment()) return false;
			}

		} while (ch != '\n' && !eof);
		++lineno;
		next();
		return true;
	} else if (ch == '*') { // C style comment
		char pr = -1;
		do
		{
			pr = ch;
			next();
			if (ch == '\n') ++lineno;

			// parse nested comments
			if (pr == '/' && (ch == '/' || ch == '*'))
			{
				if (!eatComment()) return false;
			}

		} while (!eof && !(pr == '*' && ch == '/'));
		next();

		return true;
	} else {
		return false;
	}

}




