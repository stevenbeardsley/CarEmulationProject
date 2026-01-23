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