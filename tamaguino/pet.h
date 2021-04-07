#ifndef _PET_CLASS_
#define _PET_CLASS_

#include <Arduino.h>

class Pet
{
private:
    String m_name;
    float m_hunger = 100;
    float m_happiness = 100;
    float m_health = 100;
    float m_discipline = 100;
    float m_weight = 1;
    float m_age = 0;
    bool m_isAlive = true;
    bool m_isSleeping = false;
    float m_poopometer = 0;
    int m_poops[3] = {0, 0, 0};

public:
    Pet(String name);
    String GetName();
    float GetHunger();
    float GetHappiness();
    float GetHealth();
    float GetDiscipline();
    float GetWeight();
    float GetAge();
    bool GetIsAlive();
    bool GetIsSleeping();
    float GetPoopometer();
    void SetHunger(float hunger);
    void SetHappiness(float happiness);
    void SetHealth(float health);
    void SetDiscipline(float discipline);
    void SetWeight(float weight);
    void SetAge(float age);

    void Lifecycle();
    void AddToDiscipline(float value);
    void AddToHappiness(float value, bool onlyIfLimitsNotReached = false);
    void AddToHealth(float value);
    void AddToWeight(float value);
    void Feed(float value);
    void AddToPoopometer(float value);
    int CountPoops();
    void SetPoops(long value);
    void ToggleIsSleeping();
    void SetIsSleeping(bool sleeping);
    int GetPoopsAt(int index);
    void ResetPoops();
};

#endif // _PET_CLASS_