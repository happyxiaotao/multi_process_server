#include "PcManager.h"
PcManager::PcManager() {}
PcManager::~PcManager() {}

size_t PcManager::Size()
{
   return m_mapPc.size();
}
void PcManager::AddPc(const PcSessionPtr &pc)
{
   m_mapPc[pc->GetSessionId()] = pc;
}
PcSessionPtr PcManager::GetCar(session_id_t session_id)
{
   auto iter = m_mapPc.find(session_id);
   if (iter == m_mapPc.end())
   {
      return nullptr;
   }
   return iter->second;
}
void PcManager::DelPc(const PcSessionPtr &pc)
{
   m_mapPc.erase(pc->GetSessionId());
}
void PcManager::DelPc(session_id_t session_id)
{
   m_mapPc.erase(session_id);
}