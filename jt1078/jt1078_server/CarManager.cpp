#include "CarManager.h"

CarManager::CarManager()
{
}
CarManager::~CarManager()
{
}

size_t CarManager::Size()
{
    return m_mapCar.size();
}
void CarManager::AddCar(const CarSessionPtr &car)
{
    m_mapCar[car->GetSessionId()] = car;
}
CarSessionPtr CarManager::GetCar(session_id_t session_id)
{
    auto iter = m_mapCar.find(session_id);
    if (iter == m_mapCar.end())
    {
        return nullptr;
    }
    return iter->second;
}

void CarManager::DelCar(CarSessionPtr &car, CarDisconnectCause cause)
{
    car->SetDisconnectCause(cause);
    m_mapCar.erase(car->GetSessionId());
}

void CarManager::DelCar(session_id_t session_id, CarDisconnectCause cause)
{
    auto iter = m_mapCar.find(session_id);
    if (iter == m_mapCar.end())
    {
        return;
    }
    iter->second->SetDisconnectCause(cause);
    m_mapCar.erase(iter);
}

void CarManager::DelCarByDeviceId(device_id_t device_id, CarDisconnectCause cause)
{
    for (auto iter = m_mapCar.begin(); iter != m_mapCar.end();)
    {
        if (iter->second->GetDeviceId() == device_id)
        {
            iter->second->SetDisconnectCause(cause);
            iter = m_mapCar.erase(iter);
        }
        else
        {
            ++iter;
        }
    }
}
