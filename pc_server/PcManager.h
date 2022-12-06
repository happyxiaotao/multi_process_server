#ifndef PC_MANAGER_H
#define PC_MANAGER_H
#include <map>
#include "PcSession.h"
class PcManager
{
public:
    PcManager();
    ~PcManager();

public:
    size_t Size();
    void AddPc(const PcSessionPtr &pc);
    PcSessionPtr GetCar(session_id_t session_id);
    void DelPc(const PcSessionPtr &pc);
    void DelPc(session_id_t session_id);

private:
    std::map<session_id_t, PcSessionPtr> m_mapPc;
};

#endif // PC_MANAGER_H