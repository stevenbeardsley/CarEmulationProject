#ifndef SHARED_BIT_PARSER_ERROR_H
#define SHARED_BIT_PARSER_ERROR_H

namespace shared::bit_parser
{

enum class Error 
{
    None = 0,
    OutOfRange,
    BadVarUInt,
    Misaligned,
};

}

#endif 