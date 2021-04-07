#include "pet.h"

Pet::Pet(String name) : m_name(name)
{
}

String Pet::GetName()
{
    return m_name;
}

float Pet::GetHunger()
{
    return m_hunger;
}

void Pet::SetHunger(float hunger)
{
    m_hunger = hunger;
}

float Pet::GetHappiness()
{
    return m_happiness;
}
void Pet::SetHappiness(float happiness)
{
    m_happiness = happiness;
}
void Pet::SetHealth(float health)
{
    m_health = health;
}
void Pet::SetDiscipline(float discipline)
{
    m_discipline = discipline;
}
void Pet::SetWeight(float weight)
{
    m_weight = weight;
}
void Pet::SetAge(float age)
{
    m_age = age;
}
float Pet::GetHealth()
{
    return m_health;
}

float Pet::GetDiscipline()
{
    return m_discipline;
}

float Pet::GetWeight()
{
    return m_weight;
}

float Pet::GetAge()
{
    return m_age;
}

bool Pet::GetIsAlive()
{
    return m_isAlive;
}

bool Pet::GetIsSleeping()
{
    return m_isSleeping;
}

void Pet::SetIsSleeping(bool sleeping)
{
    m_isSleeping = sleeping;
}

float Pet::GetPoopometer()
{
    return m_poopometer;
}

void Pet::Lifecycle()
{
    if (m_isAlive)
    {
        // TODO: different gradients regarding to age
        if (m_isSleeping)
        {
            m_hunger -= 0.00005;
            m_poopometer += 0.00005;
            if (m_happiness - 0.0001 > 0)
            {
                m_happiness -= 0.0001;
            }
            m_health -= 0.00005 + CountPoops() * 0.0001;
            if (m_discipline - 0.0001 > 0)
            {
                m_discipline -= 0.0001;
            }
        }
        else
        {
            m_hunger -= 0.00025;
            m_poopometer += 0.00025;
            if (m_happiness - 0.0002 > 0)
            {
                m_happiness -= 0.0002;
            }
            m_health -= 0.0001 + CountPoops() * 0.0001;
            if (m_discipline - 0.0002 > 0)
            {
                m_discipline -= 0.0002;
            }
            //discipline-=0.02;
        }
        m_age += 0.0000025;

        //diarrhea :) for testing
        //poopometer+=0.005;

        //health-=1;
        //health-=countPoops()*0.0001;
        //health-=countPoops()*0.05;
    }

    if (m_hunger <= 0 || m_health <= 0 || m_happiness <= 0)
    {
        // AHHHHH!!! das tier stirbt
        m_isAlive = false;
    }
}

void Pet::AddToDiscipline(float value)
{
    m_discipline += value;
    if (m_discipline > 100.0)
        m_discipline = 100.0;
    else if (m_discipline < 0.0)
        m_discipline = 0.0;
}

// im Orignial wurde hier nichts abgezogen, wenn der Wert dadurch auf 0 fallen wuerde
void Pet::AddToHappiness(float value, bool onlyIfLimitsNotReached)
{
    if (onlyIfLimitsNotReached)
    {
        float temp = m_happiness;
        temp += value;
        if ((temp > 0) && (temp < 100.0))
            m_happiness += value;
    }
    else
    {
        m_happiness += value;
        if (m_happiness > 100.0)
            m_happiness = 100.0;
        else if (m_happiness < 0.0)
            m_happiness = 0.0;
    }
}

void Pet::AddToHealth(float value)
{
    m_health += value;
    if (m_health > 100.0)
        m_health = 100.0;
    else if (m_health < 0.0)
        m_health = 0.0;
}

void Pet::AddToWeight(float value)
{
    m_weight += value;
    if (m_weight > 100.0)
        m_weight = 100.0;
    else if (m_weight < 0.0)
        m_weight = 0.0;
}

void Pet::Feed(float value)
{
    m_hunger += value;
    if (m_hunger > 100.0)
        m_hunger = 100.0;
    else if (m_hunger < 0.0)
        m_hunger = 0.0;
}

void Pet::AddToPoopometer(float value)
{
    m_poopometer += value;
}

int Pet::CountPoops()
{
    int poopsCnt = 0;
    for (int i = 0; i < 3; i++)
    {
        if (m_poops[i] > 0)
        {
            ++poopsCnt;
        }
    }
    return poopsCnt;
}

void Pet::SetPoops(long value)
{
    m_poopometer = CountPoops();
    // ESP32 uses std's round function which returns a double
    m_poops[(int)round(m_poopometer)] = value;
    m_poopometer = 0;
}

void Pet::ToggleIsSleeping()
{
    m_isSleeping = !m_isSleeping;
}

int Pet::GetPoopsAt(int index)
{
    if ((index < 0) || (index > 3))
        return 0;
    return m_poops[index];
}

void Pet::ResetPoops()
{
    for (int i = 0; i < 3; i++)
    {
        m_poops[i] = 0;
    }
}