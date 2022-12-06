#ifndef PUB_SUB_MESSAGE_H
#define PUB_SUB_MESSAGE_H
namespace pub_sub
{
    class Message
    {
    public:
        Message() {}
        Message(const std::string &msg) : m_msg(msg) {}

    public:
        const std::string &GetMsg() const { return m_msg; }

    private:
        std::string m_msg;
    };
} // namespace pub_sub

#endif // PUB_SUB_MESSAGE_H