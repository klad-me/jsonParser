#include "json.h"

#include <string.h>
#include <ctype.h>
#include <limits.h>


static void skip_ws(const char **str, int *len)
{
	while ( ((*len) > 0) && (isspace(**str)) )
	{
		(*str)++;
		(*len)--;
	}
}


static int expect(const char **str, int *len, char c)
{
	skip_ws(str, len);
	if ( ((*len) > 0) && ((**str) == c) )
	{
		(*str)++;
		(*len)--;
		return 1;
	} else
	{
		return 0;
	}
}


static int skip_ident(const char **str, int *len, const char *ident)
{
	int l=strlen(ident);
	
	// Проверим, что идентификатор подходит
	if ( ((*len) >= l) && (strncmp(*str, ident, l) == 0) )
	{
		(*str)+=l;
		(*len)-=l;
		return 1;
	} else
	{
		return 0;
	}
}


static void skip_digits(const char **str, int *len)
{
	while ( ((*len) > 0) && (isdigit(**str)) )
	{
		(*str)++;
		(*len)--;
	}
}


static int skip_number(const char **str, int *len)
{
	// Пропускаем знак
	expect(str, len, '-');
	
	// Должна быть целая часть или десятичная точка
	if ( ((*len) < 1) || ( (! isdigit(**str)) && ((**str) != '.') ) )
		return 0;
	
	// Пропускаем целую часть
	skip_digits(str, len);
	
	// Пропускаем дробную часть
	if ( ((*len) > 0) && ((**str) == '.') )
	{
		(*str)++;
		(*len)--;
		skip_digits(str, len);
	}
	
	return 1;
}


static int skip_string(const char **str, int *len)
{
	// Проверим, чтобы строка начиналась с кавычки
	if (! expect(str, len, '\"')) return 0;
	
	// Пропускаем символы
	while ((*len) > 0)
	{
		switch (**str)
		{
			case '\\':
				// escape
				(*str)++;
				(*len)--;
				if ((*len) < 1) return 0;
				switch (**str)
				{
					case '\"':
					case '\\':
					case '/':
					case 't':
					case 'n':
					case 'r':
					case 'f':
					case 'b':
						// Просто пропускаем
						break;
					
					default:
						// Неверный escape
						return 0;
				}
				break;
			
			case '\"':
				// Конец строки
				(*str)++;
				(*len)--;
				return 1;
			
			default:
				// Просто символ
				break;
		}
		
		// Пропускаем символ
		(*str)++;
		(*len)--;
	}
	
	// Не найдена закрывающая кавычка
	return 0;
}


static int skip_value(const char **str, int *len)
{
	skip_ws(str, len);
	
	if ((*len) < 1) return 0;
	switch (**str)
	{
		case 'n':
			// null
			return skip_ident(str, len, "null");
		
		case 'f':
			// false
			return skip_ident(str, len, "false");
		
		case 't':
			// true
			return skip_ident(str, len, "true");
		
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case '-':
		case '.':
			// number
			return skip_number(str, len);
		
		case '\"':
			// string
			return skip_string(str, len);
		
		case '[':
			// array
			(*str)++;
			(*len)--;
			
			// Проверим на пустой массив
			if (expect(str, len, ']')) return 1;
			
			do
			{
				// Должно быть значение
				if (! skip_value(str, len)) return 0;
			} while (expect(str, len, ','));
			
			return expect(str, len, ']');
		
		case '{':
			// object
			(*str)++;
			(*len)--;
			
			// Проверим на пустой объект
			if (expect(str, len, '}')) return 1;
			
			do
			{
				// Должен быть ключ и значение
				if (! skip_string(str, len)) return 0;
				if (! expect(str, len, ':')) return 1;
				if (! skip_value(str, len)) return 0;
			} while (expect(str, len, ','));
			
			return expect(str, len, '}');
		
		default:
			// incorrect
			return 0;
	}
}


static int validate(const char *str, int len)
{
	// Пропускаем пробелы
	skip_ws(&str, &len);
	
	// JSON должен начинаться с объекта
	if ( (len < 2) || ((*str) != '{') ) return 0;
	
	// Пропускаем тело объекта
	if (! skip_value(&str, &len)) return 0;
	
	// Пропускаем пробелы
	skip_ws(&str, &len);
	
	// Не должно остаться символов
	return (len==0);
}


jsonValue jsonParse(const char *str, int len)
{
	// Проверяем json
	if (! validate(str, len)) return (jsonValue){ type: JSON_NULL };
	
	// Перемещаемся на первый ключ
	while (isspace(*str)) str++;
	str++;	// пропускаем {
	while (isspace(*str)) str++;
	
	return (jsonValue){ type: JSON_OBJECT, ptr: str };
}


int jsonBoolean(jsonValue j)
{
	switch (j.type)
	{
		case JSON_TRUE:
			return 1;
		
		case JSON_NUMBER:
			return jsonDouble(j) != 0.0;
		
		default:
			return 0;
	}
}


int jsonInteger(jsonValue j)
{
	const char *s;
	
	switch (j.type)
	{
		case JSON_TRUE:
			return 1;
		
		case JSON_NUMBER:
			s=j.ptr;
			break;
		
		case JSON_STRING:
			s=j.ptr+1;
			break;
		
		default:
			return 0;
	}
	
	// Получаем знак
	int sign;
	if ((*s)=='-')
	{
		sign=-1;
		s++;
	} else
	{
		sign=1;
	}
	
	// Получаем число
	int value=0;
	while (isdigit(*s))
	{
		value=value*10 + ((*s++)-'0');
	}
	
	return sign*value;
}


double jsonDouble(jsonValue j)
{
	const char *s;
	
	switch (j.type)
	{
		case JSON_TRUE:
			return 1.0;
		
		case JSON_NUMBER:
			s=j.ptr;
			break;
		
		case JSON_STRING:
			s=j.ptr+1;
			break;
		
		default:
			return 0.0;
	}
	
	while (isspace(*s)) s++;
	
	// Получаем знак
	double sign;
	if ((*s)=='-')
	{
		sign=-1.0;
		s++;
	} else
	{
		sign=1.0;
	}
	
	// Получаем целую часть
	double value=0.0;
	while (isdigit(*s))
	{
		value=value*10.0 + (double)((*s++)-'0');
	}
	
	// Получаем дробную часть
	if ((*s++)=='.')
	{
		double mul=0.1;
		while (isdigit(*s))
		{
			value+=mul * (double)((*s++)-'0');
			mul/=10.0;
		}
	}
	
	return sign*value;
}


int jsonString(jsonValue j, char *value, int max)
{
	if (j.type != JSON_STRING)
	{
		if ( (value) && (max > 0) )
			(*value)=0;
		return 0;
	}
	
	const char *s=j.ptr+1;	// пропускаем кавычку
	int len=0;
	while (1)
	{
		char c=(*s++);
		switch (c)
		{
			case '\\':
				// escape
				c=(*s++);
				switch (c)
				{
					case 't':	c='\t'; break;
					case 'n':	c='\n'; break;
					case 'r':	c='\r'; break;
					case 'f':	c='\f'; break;
					case 'b':	c='\b'; break;
					default:			break;
				}
				break;
			
			case '\"':
				// Конец строки
				if ( (value) && (max > 0) )
					(*value)=0;
				return len;
			
			default:
				// Просто символ
				break;
		}
		
		if ( (value) && (max > 1) )
		{
			(*value++)=c;
			max--;
		}
		len++;
	}
}


int jsonObjectKey(jsonValue j, char *value, int max)
{
	if ( (j.type != JSON_OBJECT) ||
		 ((*j.ptr) != '\"') )
	{
		if ( (value) && (max > 0) )
			(*value)=0;
		return 0;
	}
	
	return jsonString((jsonValue){ type: JSON_STRING, ptr: j.ptr }, value, max);
}


static void guessType(jsonValue *j)
{
	switch (*j->ptr)
	{
		case 'n':
			j->type=JSON_NULL;
			break;
		
		case 'f':
			j->type=JSON_FALSE;
			break;
		
		case 't':
			j->type=JSON_TRUE;
			break;
		
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case '-':
		case '.':
			j->type=JSON_NUMBER;
			break;
		
		case '\"':
			j->type=JSON_STRING;
			break;
		
		case '[':
			j->type=JSON_ARRAY;
			j->ptr++;
			while (isspace(*j->ptr)) j->ptr++;
			break;
		
		case '{':
			j->type=JSON_OBJECT;
			j->ptr++;
			while (isspace(*j->ptr)) j->ptr++;
			break;
		
		default:
			j->type=JSON_NULL;
			break;
	}
}


jsonValue jsonObjectValue(jsonValue j)
{
	if (j.type == JSON_OBJECT)
	{
		int len=INT_MAX;
		if (! skip_string(&j.ptr, &len)) return (jsonValue){ type: JSON_NULL };
		j.ptr++;	// пропускаем :
		while (isspace(*j.ptr)) j.ptr++;
	}
	
	guessType(&j);
	return j;
}


static int jsonMatchString(const char *s, const char *value)
{
	int len=0;
	while (1)
	{
		char c=(*s++);
		switch (c)
		{
			case '\\':
				// escape
				c=(*s++);
				switch (c)
				{
					case 't':	c='\t'; break;
					case 'n':	c='\n'; break;
					case 'r':	c='\r'; break;
					case 'f':	c='\f'; break;
					case 'b':	c='\b'; break;
					default:			break;
				}
				break;
			
			case '\"':
				// Конец строки
				return (*value)==0;
			
			default:
				// Просто символ
				break;
		}
		
		if (c != (*value++)) return 0;
	}
}


jsonValue jsonObjectValueByKey(jsonValue j, const char *key)
{
	if ( (j.type != JSON_OBJECT) ||
		 ((*j.ptr) != '\"') )
		return (jsonValue){ type: JSON_NULL };
	
	int len=INT_MAX;
	do
	{
		// Пропускаем пробелы
		while (isspace(*j.ptr)) j.ptr++;
		
		// Проверяем ключ
		if (jsonMatchString(j.ptr+1, key))
		{
			skip_string(&j.ptr, &len);
			expect(&j.ptr, &len, ':');
			while (isspace(*j.ptr)) j.ptr++;
			
			guessType(&j);
			return j;
		}
		
		// Пропускаем ключ
		skip_string(&j.ptr, &len);
		expect(&j.ptr, &len, ':');
		skip_value(&j.ptr, &len);
	} while (expect(&j.ptr, &len, ','));
	
	return (jsonValue){ type: JSON_NULL };
}


jsonValue jsonNext(jsonValue j)
{
	int len=INT_MAX;
	
	if (j.type == JSON_ARRAY)
	{
		if ((*j.ptr) != ']')
		{
			skip_value(&j.ptr, &len);
			if (expect(&j.ptr, &len, ','))
			{
				while (isspace(*j.ptr)) j.ptr++;
				return j;
			}
		}
	} else
	if (j.type == JSON_OBJECT)
	{
		if ((*j.ptr) != '}')
		{
			skip_string(&j.ptr, &len);
			expect(&j.ptr, &len, ':');
			skip_value(&j.ptr, &len);
			if (expect(&j.ptr, &len, ','))
			{
				while (isspace(*j.ptr)) j.ptr++;
				return j;
			}
		}
	}
	
	return (jsonValue){ type: JSON_NULL };
}


#ifdef JSON_TESTS

#include <stdio.h>


static void printArray(jsonValue j, int indent);
static void printObject(jsonValue j, int indent);


static void printValue(jsonValue j, int indent)
{
	char str[100];
	
	//printf("value ptr='%s'\n", j.ptr);
	
	switch (j.type)
	{
		case JSON_NULL:
			printf("%*snull\n", indent, "");
			break;
		
		case JSON_FALSE:
			printf("%*sfalse\n", indent, "");
			break;
		
		case JSON_TRUE:
			printf("%*sfalse\n", indent, "");
			break;
		
		case JSON_NUMBER:
			printf("%*s%.6f\n", indent, "", jsonDouble(j));
			break;
		
		case JSON_STRING:
			jsonString(j, str, sizeof(str));
			printf("%*s\"%s\"\n", indent, "", str);
			break;
		
		case JSON_ARRAY:
			printf("%*sArray:\n", indent, "");
			printArray(j, indent+2);
			break;
		
		case JSON_OBJECT:
			printf("%*sObject:\n", indent, "");
			printObject(j, indent+2);
			break;
	}
}


static void printArray(jsonValue j, int indent)
{
	int i=0;
	//printf("array ptr='%s'\n", j.ptr);
	while (j.type == JSON_ARRAY)
	{
		printf("%*s[%d]\n", indent+2, "", i);
		printValue(jsonObjectValue(j), indent+4);
		
		j=jsonNext(j);
	}
}


static void printObject(jsonValue j, int indent)
{
	char key[100];
	//printf("object ptr='%s'\n", j.ptr);
	while (j.type == JSON_OBJECT)
	{
		jsonObjectKey(j, key, sizeof(key));
		printf("%*sKey '%s'\n", indent+2, "", key);
		printValue(jsonObjectValue(j), indent+4);
		
		j=jsonNext(j);
	}
}


int main()
{
	const char* types[]=
	{
		"null",
		"false",
		"true",
		"number",
		"string",
		"array",
		"object"
	};
	
	char json[65536];
	
#if 1
	FILE *f=fopen("test.json", "r");
	if (! f)
	{
		perror("test.json");
		return -1;
	}
	int len=fread(json, 1, sizeof(json), f);
	fclose(f);
#else
	strcpy(json, "{\"hello\": {\"wow\": 1, \"bau\": 2}, \"world\": [1,2,3]}");
	int len=strlen(json);
#endif
	
	
	
	jsonValue j=jsonParse(json, len);
	printf("result=%s\n", types[j.type]);
	printObject(j, 0);
	
	return 0;
}


#endif
