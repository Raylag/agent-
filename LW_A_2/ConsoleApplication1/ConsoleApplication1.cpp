#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <random>
#include <string>
#include <chrono>

using namespace std;

// Класс агента
class Agent {
private:
    int id;
    set<string> targetPatents;      // Целевые патенты
    set<string> currentPatents;     // Текущие патенты
    int communicationRounds;        // Количество раундов общения
    int successfulExchanges;        // Количество успешных обменов
    bool targetCompleted;           // Собрал ли целевой набор

public:
    Agent(int agentId, const set<string>& target)
        : id(agentId), targetPatents(target), communicationRounds(0),
        successfulExchanges(0), targetCompleted(false) {
    }

    // Добавить начальные патенты
    void addInitialPatents(const set<string>& initialPatents) {
        currentPatents.insert(initialPatents.begin(), initialPatents.end());
        checkTargetCompletion();
    }

    // Проверить, собран ли целевой набор
    void checkTargetCompletion() {
        targetCompleted = true;
        for (const auto& patent : targetPatents) {
            if (currentPatents.find(patent) == currentPatents.end()) {
                targetCompleted = false;
                break;
            }
        }
    }

    // Получить список нужных патентов, которых нет у агента
    vector<string> getNeededPatents() const {
        vector<string> needed;
        for (const auto& patent : targetPatents) {
            if (currentPatents.find(patent) == currentPatents.end()) {
                needed.push_back(patent);
            }
        }
        return needed;
    }

    // Обмен с другим агентом
    bool exchangeWith(Agent& other) {
        communicationRounds++;
        other.communicationRounds++;

        // Если текущий агент собрал все, он может отдавать безвозмездно
        if (targetCompleted) {
            // Найти патент, который нужен другому агенту и есть у текущего
            auto otherNeeded = other.getNeededPatents();
            for (const auto& patent : otherNeeded) {
                if (currentPatents.find(patent) != currentPatents.end()) {
                    // Безвозмездная передача
                    other.currentPatents.insert(patent);
                    other.checkTargetCompletion();
                    return true;
                }
            }
            return false;
        }

        // Найти патент, который нужен текущему агенту и есть у другого
        auto needed = getNeededPatents();
        for (const auto& patent : needed) {
            if (other.currentPatents.find(patent) != other.currentPatents.end()) {

                // Если другой агент собрал все, он отдаст безвозмездно
                if (other.targetCompleted) {
                    currentPatents.insert(patent);
                    checkTargetCompletion();
                    successfulExchanges++;
                    return true;
                }

                // Иначе нужен обмен: найти патент, который нужен другому агенту
                auto otherNeeded = other.getNeededPatents();
                for (const auto& otherPatent : otherNeeded) {
                    if (currentPatents.find(otherPatent) != currentPatents.end()) {
                        // Обмен патентами
                        currentPatents.insert(patent);
                        currentPatents.erase(otherPatent);

                        other.currentPatents.insert(otherPatent);
                        other.currentPatents.erase(patent);

                        checkTargetCompletion();
                        other.checkTargetCompletion();

                        successfulExchanges++;
                        other.successfulExchanges++;
                        return true;
                    }
                }
                break; // Нашли нужный патент, но нет подходящего для обмена
            }
        }

        return false; // Обмен не состоялся
    }

    // Геттеры
    int getId() const { return id; }
    int getCommunicationRounds() const { return communicationRounds; }
    int getSuccessfulExchanges() const { return successfulExchanges; }
    int getTargetSize() const { return targetPatents.size(); }
    bool isTargetCompleted() const { return targetCompleted; }
    const set<string>& getCurrentPatents() const { return currentPatents; }
    const set<string>& getTargetPatents() const { return targetPatents; }
};

// Класс системы моделирования
class PatentSystem {
private:
    vector<Agent> agents;
    mt19937 rng;
    int totalCommunicationRounds;

public:
    PatentSystem() : rng(chrono::steady_clock::now().time_since_epoch().count()),
        totalCommunicationRounds(0) {
    }

    // Генерация уникального ID патента
    string generatePatentId(int agentId, int patentNum) {
        return "Patent_A" + to_string(agentId) + "_P" + to_string(patentNum);
    }

    // Генерация начальных условий
    void generateInitialConditions(int numAgents, int targetSize, int initialSetSize) {
        agents.clear();

        // Шаг 1: Генерация целевых наборов для каждого агента
        vector<set<string>> targets(numAgents);
        for (int i = 0; i < numAgents; i++) {
            for (int j = 0; j < targetSize; j++) {
                targets[i].insert(generatePatentId(i, j));
            }
        }

        // Шаг 2: Объединение всех целевых наборов
        set<string> allPatents;
        for (const auto& targetSet : targets) {
            allPatents.insert(targetSet.begin(), targetSet.end());
        }

        // Шаг 3: Добавление дополнительных (нецелевых) патентов
        int additionalPatents = numAgents * targetSize / 2;
        for (int i = 0; i < additionalPatents; i++) {
            allPatents.insert("Extra_P" + to_string(i));
        }

        // Шаг 4: Создание агентов
        for (int i = 0; i < numAgents; i++) {
            agents.emplace_back(i, targets[i]);
        }

        // Шаг 5: Равномерная раздача патентов
        vector<string> allPatentsVec(allPatents.begin(), allPatents.end());
        shuffle(allPatentsVec.begin(), allPatentsVec.end(), rng);

        // Раздача патентов агентам
        int patentIndex = 0;
        int patentsPerAgent = (allPatentsVec.size() < numAgents * initialSetSize) ?
            allPatentsVec.size() / numAgents : initialSetSize;

        for (auto& agent : agents) {
            set<string> initialSet;
            for (int j = 0; j < patentsPerAgent && patentIndex < allPatentsVec.size(); j++) {
                initialSet.insert(allPatentsVec[patentIndex++]);
            }
            agent.addInitialPatents(initialSet);
        }

        // Дополнительная раздача оставшихся патентов для баланса
        while (patentIndex < allPatentsVec.size()) {
            int agentIndex = patentIndex % numAgents;
            agents[agentIndex].addInitialPatents({ allPatentsVec[patentIndex++] });
        }
    }

    // Проверка завершения симуляции
    bool isSimulationComplete() const {
        for (const auto& agent : agents) {
            if (!agent.isTargetCompleted()) {
                return false;
            }
        }
        return true;
    }

    // Запуск симуляции
    void runSimulation() {
        totalCommunicationRounds = 0;
        int maxIterations = 10000; // Предохранитель от бесконечного цикла

        while (!isSimulationComplete() && maxIterations-- > 0) {
            // Перемешиваем агентов для случайного порядка общения
            shuffle(agents.begin(), agents.end(), rng);

            // Каждый агент пытается пообщаться с каждым другим агентом
            for (size_t i = 0; i < agents.size(); i++) {
                if (agents[i].isTargetCompleted()) continue;

                for (size_t j = 0; j < agents.size(); j++) {
                    if (i == j) continue;

                    // Попытка обмена
                    bool exchanged = agents[i].exchangeWith(agents[j]);
                    if (exchanged) {
                        totalCommunicationRounds++;
                    }
                }
            }
        }

        if (maxIterations <= 0) {
            cout << "Предупреждение: достигнуто максимальное количество итераций!" << endl;
        }
    }

    // Вывод результатов
    void printResults() const {
        cout << "\n=== РЕЗУЛЬТАТЫ СИМУЛЯЦИИ ===" << endl;
        cout << "Всего агентов: " << agents.size() << endl;
        cout << "Всего раундов общения в системе: " << totalCommunicationRounds << endl;
        cout << "Все агенты собрали целевые наборы: "
            << (isSimulationComplete() ? "Да" : "Нет") << endl << endl;

        cout << "Детальная информация по агентам:" << endl;
        cout << "ID | Размер целевого набора | Успешных обменов | Раундов общения" << endl;
        cout << "---|------------------------|------------------|----------------" << endl;

        for (const auto& agent : agents) {
            cout << agent.getId() << "  | "
                << agent.getTargetSize() << "                    | "
                << agent.getSuccessfulExchanges() << "                | "
                << agent.getCommunicationRounds() << endl;
        }
    }

    // Дополнительная статистика
    void printDetailedStatistics() const {
        cout << "\n=== ДЕТАЛЬНАЯ СТАТИСТИКА ===" << endl;

        int totalExchanges = 0;
        int totalRounds = 0;
        int completedAgents = 0;

        for (const auto& agent : agents) {
            totalExchanges += agent.getSuccessfulExchanges();
            totalRounds += agent.getCommunicationRounds();
            if (agent.isTargetCompleted()) completedAgents++;
        }

        cout << "Среднее количество обменов на агента: "
            << (double)totalExchanges / agents.size() << endl;
        cout << "Среднее количество раундов на агента: "
            << (double)totalRounds / agents.size() << endl;
        cout << "Агентов, собравших целевые наборы: "
            << completedAgents << " из " << agents.size() << endl;
    }
};

// Основная функция для демонстрации
int main() {
    setlocale(LC_ALL, "Russian");

    PatentSystem system;

    // Параметры симуляции
    int numAgents = 10;         // Количество агентов
    int targetSize = 5;         // Размер целевого набора каждого агента
    int initialSetSize = 3;     // Примерный начальный размер набора

    cout << "Генерация начальных условий..." << endl;
    system.generateInitialConditions(numAgents, targetSize, initialSetSize);

    cout << "Запуск симуляции..." << endl;
    system.runSimulation();

    system.printResults();
    system.printDetailedStatistics();

    // Пример дополнительной симуляции с другими параметрами
    cout << "\n\n=== ДОПОЛНИТЕЛЬНАЯ СИМУЛЯЦИЯ ===" << endl;
    cout << "Параметры: 20 агентов, целевой набор 7, начальный набор 4" << endl;

    PatentSystem system2;
    system2.generateInitialConditions(20, 7, 4);
    system2.runSimulation();
    system2.printResults();

    return 0;
}