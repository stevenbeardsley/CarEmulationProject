#ifndef SHARED_CAN_MESSAGE_CATEGORY_H
#define SHARED_CAN_MESSAGE_CATEGORY_H

namespace shared::can
{

enum class MessageCategory
{
    Control,
    Status,
    Error
};

}

#endif 