#ifndef JT1078_SERVER_CAR_MANAGER_H
#define JT1078_SERVER_CAR_MANAGER_H

#include <map>
#include "CarSession.h"

typedef uint64_t session_id_t;
class CarManager
{
public:
    CarManager();
    ~CarManager();

public:
    size_t Size();
    void AddCar(const CarSessionPtr &car);
    CarSessionPtr GetCar(session_id_t session_id);
    void DelCar(CarSessionPtr &car, CarDisconnectCause cause);
    void DelCar(session_id_t session_id, CarDisconnectCause cause);
    void DelCarByDeviceId(device_id_t device_id, CarDisconnectCause cause);

private:
    std::map<session_id_t, CarSessionPtr> m_mapCar;
};

#endif // JT1078_SERVER_CAR_MANAGER_H