#include "json.h"

#include <string.h>
#include <ctype.h>
#include <limits.h>


#define C(ptr)	((ptr)->cb((ptr)->cb_param, (ptr)->pos++))
#define U(ptr)	do { (ptr)->pos--; } while(0)


static void skip_ws(jsonPtr *ptr)
{
	while (isspace(C(ptr))) ;
	U(ptr);
}


static bool expect(jsonPtr *ptr, char c)
{
	skip_ws(ptr);
	if (C(ptr) == c) return true;
	U(ptr);
	return false;
}


static bool skip_ident(jsonPtr *ptr, const char *ident)
{
	while ( (*ident) && (C(ptr) == (*ident)) )
		ident++;
	
	return ((*ident) == 0);
}


static bool skip_digits(jsonPtr *ptr)
{
	bool ok=false;
	
	while (isdigit(C(ptr)))
		ok=true;
	U(ptr);
	
	return ok;
}


static bool skip_number(jsonPtr *ptr)
{
	// Пропускаем знак
	expect(ptr, '-');
	
	// Должна быть целая часть
	if (! skip_digits(ptr)) return false;
	
	// Пропускаем дробную часть
	if (expect(ptr, '.'))
	{
		if (! skip_digits(ptr)) return false;
	}
	
	return true;
}


static bool skip_string(jsonPtr *ptr)
{
	// Проверим, чтобы строка начиналась с кавычки
	if (! expect(ptr, '\"')) return false;
	
	// Пропускаем символы
	while (1)
	{
		switch (C(ptr))
		{
			case 0:
				// Неожиданный конец
				return false;
			
			case '\\':
				// escape
				switch (C(ptr))
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
						return false;
				}
				break;
			
			case '\"':
				// Конец строки
				return true;
			
			default:
				// Просто символ
				break;
		}
	}
}


static bool skip_value(jsonPtr *ptr)
{
	skip_ws(ptr);
	
	char c=C(ptr); U(ptr);
	switch (c)
	{
		case 'n':
			// null
			return skip_ident(ptr, "null");
		
		case 'f':
			// false
			return skip_ident(ptr, "false");
		
		case 't':
			// true
			return skip_ident(ptr, "true");
		
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
			// number
			return skip_number(ptr);
		
		case '\"':
			// string
			return skip_string(ptr);
		
		case '[':
			// array
			ptr->pos++;	// пропускаем '['
			
			// Проверим на пустой массив
			if (expect(ptr, ']')) return true;
			
			do
			{
				// Должно быть значение
				if (! skip_value(ptr)) return false;
			} while (expect(ptr, ','));
			
			return expect(ptr, ']');
		
		case '{':
			// object
			ptr->pos++;	// пропускаем '{'
			
			// Проверим на пустой объект
			if (expect(ptr, '}')) return true;
			
			do
			{
				// Должен быть ключ и значение
				if (! skip_string(ptr)) return false;
				if (! expect(ptr, ':')) return false;
				if (! skip_value(ptr)) return false;
			} while (expect(ptr, ','));
			
			return expect(ptr, '}');
		
		default:
			// incorrect
			return false;
	}
}


static bool validate(json_getc cb, void *cb_param)
{
	jsonPtr ptr=(jsonPtr){ cb: cb, cb_param: cb_param, pos: 0 };
	
	// Пропускаем пробелы
	skip_ws(&ptr);
	
	// JSON должен начинаться с объекта
	if (C(&ptr) != '{') return false;
	U(&ptr);
	
	// Пропускаем тело объекта
	if (! skip_value(&ptr)) return false;
	
	// Пропускаем пробелы
	skip_ws(&ptr);
	
	// Не должно остаться символов
	return (C(&ptr) == 0);
}


jsonValue jsonParse(json_getc cb, void *cb_param)
{
	// Проверяем json
	if (! validate(cb, cb_param)) return (jsonValue){ type: JSON_NULL };
	
	// Перемещаемся на первый ключ
	jsonValue v=(jsonValue){ type: JSON_OBJECT, ptr: { cb: cb, cb_param: cb_param, pos: 0 } };
	skip_ws(&v.ptr);
	v.ptr.pos++;	// пропускаем {
	skip_ws(&v.ptr);
	
	return v;
}


static char string_getc(void *param, int pos)
{
	return ((const char*)param)[pos];
}


jsonValue jsonParseString(const char *str)
{
	return jsonParse(string_getc, (void*)str);
}


bool jsonBoolean(const jsonValue &j)
{
	switch (j.type)
	{
		case JSON_TRUE:
			return true;
		
		case JSON_NUMBER:
			return jsonDouble(j) != 0.0;
		
		default:
			return false;
	}
}


int jsonInteger(const jsonValue &j)
{
	int pos=j.ptr.pos;
	
	switch (j.type)
	{
		case JSON_TRUE:
			return 1;
		
		case JSON_NUMBER:
			break;
		
		case JSON_STRING:
			pos++;	// пропускаем '"'
			break;
		
		default:
			return 0;
	}
	
	// Получаем знак
	int sign;
	if (j.ptr.cb(j.ptr.cb_param, pos) == '-')
	{
		sign=-1;
		pos++;
	} else
	{
		sign=1;
	}
	
	// Получаем число
	int value=0;
	char c;
	while (isdigit(c=j.ptr.cb(j.ptr.cb_param, pos++)))
	{
		value=value*10 + (c-'0');
	}
	
	return sign*value;
}


double jsonDouble(const jsonValue &j)
{
	int pos=j.ptr.pos;
	
	switch (j.type)
	{
		case JSON_TRUE:
			return 1.0;
		
		case JSON_NUMBER:
			break;
		
		case JSON_STRING:
			pos++;	// пропускаем '"'
			break;
		
		default:
			return 0.0;
	}
	
	// Получаем знак
	double sign;
	if (j.ptr.cb(j.ptr.cb_param, pos) == '-')
	{
		sign=-1.0;
		pos++;
	} else
	{
		sign=1.0;
	}
	
	// Получаем целую часть
	double value=0.0;
	char c;
	while (isdigit(c=j.ptr.cb(j.ptr.cb_param, pos++)))
	{
		value=value*10.0 + (double)(c-'0');
	}
	
	// Получаем дробную часть
	if (c=='.')
	{
		double mul=0.1;
		while (isdigit(c=j.ptr.cb(j.ptr.cb_param, pos++)))
		{
			value+=mul * (double)(c-'0');
			mul/=10.0;
		}
	}
	
	return sign*value;
}


static int jsonString_int(const jsonPtr &ptr, char *value, int max, int *endpos=0)
{
	int pos=ptr.pos+1;	// пропускаем '"'
	int len=0;
	while (1)
	{
		char c=ptr.cb(ptr.cb_param, pos++);
		switch (c)
		{
			case '\\':
				// escape
				c=ptr.cb(ptr.cb_param, pos++);
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
			case 0:
				// Конец строки
				if ( (value) && (max > 0) )
					(*value)=0;
				if (endpos) *endpos=pos;
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


int jsonString(const jsonValue &j, char *value, int max)
{
	if (j.type != JSON_STRING)
	{
		if ( (value) && (max > 0) )
			(*value)=0;
		return 0;
	}
	
	return jsonString_int(j.ptr, value, max);
}


int jsonObjectKey(const jsonValue &j, char *value, int max)
{
	if (j.type != JSON_OBJECT)
	{
		if ( (value) && (max > 0) )
			(*value)=0;
		return 0;
	}
	
	return jsonString_int(j.ptr, value, max);
}


static bool guessType(jsonValue *j)
{
	skip_ws(&j->ptr);
	
	switch (j->ptr.cb(j->ptr.cb_param, j->ptr.pos))
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
			j->type=JSON_NUMBER;
			break;
		
		case '\"':
			j->type=JSON_STRING;
			break;
		
		case '[':
			j->type=JSON_ARRAY;
			j->ptr.pos++;	// пропускаем '['
			skip_ws(&j->ptr);
			break;
		
		case '{':
			j->type=JSON_OBJECT;
			j->ptr.pos++;	// пропускаем '['
			skip_ws(&j->ptr);
			break;
		
		default:
			// Ошибка
			j->type=JSON_NULL;
			return false;
	}
	return true;
}


jsonValue jsonObjectValue(const jsonValue &j)
{
	jsonValue v=(jsonValue){ type: JSON_NULL, ptr: j.ptr };
	
	if (j.type == JSON_OBJECT)
	{
		// Пропускаем ключ
		jsonString_int(v.ptr, 0, 0, &v.ptr.pos);
		
		// Пропускаем ':'
		if (! expect(&v.ptr, ':')) return v;
	}
	
	// Определяем тип
	guessType(&v);
	return v;
}


static bool matchString(jsonPtr *ptr, const char *value)
{
	bool result=true;
	
	// Пропускаем '"'
	if (C(ptr) != '"') return false;
	
	while (1)
	{
		char c=C(ptr);
		switch (c)
		{
			case '\\':
				// escape
				c=C(ptr);
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
			case 0:
				// Конец строки
				return result && ((*value)==0);
			
			default:
				// Просто символ
				break;
		}
		
		if ( result && (c != (*value++)) )
			result=false;
	}
}


jsonValue jsonObjectValueByKey(const jsonValue &j, const char *key)
{
	jsonValue v=(jsonValue){ type: JSON_NULL, ptr: j.ptr };
	
	if (j.type != JSON_OBJECT) return v;
	
	while (1)
	{
		// Проверяем ключ
		bool match=matchString(&v.ptr, key);
		if (! expect(&v.ptr, ':')) break;
		
		if (match)
		{
			guessType(&v);
			break;
		}
		
		// Пропускаем значение
		if (! skip_value(&v.ptr)) break;
		
		// Пропускаем ','
		if (! expect(&v.ptr, ',')) break;
		
		// Пропускаем пробелы до следующего ключа
		skip_ws(&v.ptr);
	}
	
	return v;
}


jsonValue jsonNext(const jsonValue &j)
{
	jsonValue v=j;
	
	if (v.type == JSON_ARRAY)
	{
		// Пропускаем текущее значение
		if (! skip_value(&v.ptr)) goto fail;
		if (! expect(&v.ptr, ',')) goto fail;
		
		// Проверим, что есть следующее значение
		if (! guessType(&v)) goto fail;
		v.type=JSON_ARRAY;
		return v;
	} else
	if (v.type == JSON_OBJECT)
	{
		// Пропускаем текущее значение
		if (! skip_string(&v.ptr)) goto fail;
		if (! expect(&v.ptr, ':')) goto fail;
		if (! skip_value(&v.ptr)) goto fail;
		if (! expect(&v.ptr, ',')) goto fail;
		
		// Проверим, что есть следующее значение
		guessType(&v);
		if (v.type != JSON_STRING) goto fail;
		v.type=JSON_OBJECT;
		return v;
	}
	
fail:
	return (jsonValue){ type: JSON_NULL };
}
