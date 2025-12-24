#include <iostream>
#include <vector>
#include <queue>
#include <random>
#include <algorithm>
#include <iomanip>


// Генератор случайных чисел
class RandomGenerator {
private:
    std::mt19937 gen;
    std::uniform_real_distribution<double> time_dist;
    std::uniform_int_distribution<int> difficulty_dist;

public:
    RandomGenerator(double a, double b)
        : gen(std::random_device{}()),
        time_dist(a, b),
        difficulty_dist(1, 10) {
    }

    double getNextTime() {
        return time_dist(gen);
    }

    int getDifficulty() {
        return difficulty_dist(gen);
    }
};

// Структура для клиента
struct Client {
    int id;
    double arrival_time;
    int difficulty;

    Client(int _id, double _time, int _diff)
        : id(_id), arrival_time(_time), difficulty(_diff) {
    }
};

// Структура для события
struct Event {
    double time;
    int type; // 0 - прибытие клиента, 1 - завершение обслуживания
    int client_id;
    int agent_id;

    Event(double t, int tp, int cid, int aid = -1)
        : time(t), type(tp), client_id(cid), agent_id(aid) {
    }

    // Для приоритетной очереди (меньшее время - выше приоритет)
    bool operator>(const Event& other) const {
        return time > other.time;
    }
};

// Класс агента
class Agent {
private:
    std::queue<Client> client_queue;
    double current_load; // Текущая загрузка
    double next_free_time; // Время, когда освободится
    Client* current_client;

public:
    int id;
    int served_count;
    double total_work_time;

    Agent(int _id) : id(_id), served_count(0), total_work_time(0.0),
        current_load(0.0), next_free_time(0.0), current_client(nullptr) {
    }

    // Добавить клиента в очередь
    void addClient(const Client& client) {
        client_queue.push(client);
        updateLoad();
    }

    // Начать обслуживание следующего клиента
    bool startNextService(double current_time, std::priority_queue<Event, std::vector<Event>,
        std::greater<Event>>&events) {
        if (client_queue.empty() || current_client != nullptr) {
            return false;
        }

        current_client = new Client(client_queue.front());
        client_queue.pop();

        next_free_time = current_time + current_client->difficulty;
        served_count++;
        total_work_time += current_client->difficulty;

        // Создаем событие завершения обслуживания
        events.push(Event(next_free_time, 1, current_client->id, id));

        updateLoad();
        return true;
    }

    // Завершить текущее обслуживание
    void finishService() {
        delete current_client;
        current_client = nullptr;
        updateLoad();
    }

    // Обновить текущую загрузку
    void updateLoad() {
        current_load = 0.0;

        // Добавляем время дообслуживания текущего клиента
        if (current_client != nullptr) {
            current_load += next_free_time;
        }

        // Добавляем сложность всех клиентов в очереди
        std::queue<Client> temp_queue = client_queue;
        while (!temp_queue.empty()) {
            current_load += temp_queue.front().difficulty;
            temp_queue.pop();
        }
    }

    // Получить текущую загрузку
    double getCurrentLoad() const {
        return current_load;
    }

    // Проверить, свободен ли агент
    bool isFree(double current_time) const {
        return current_client == nullptr || next_free_time <= current_time;
    }

    // Получить размер очереди
    int getQueueSize() const {
        return client_queue.size();
    }

    // Получить время освобождения
    double getNextFreeTime() const {
        return next_free_time;
    }
};

// Класс системы
class System {
private:
    int n; // Количество агентов
    int m; // Количество клиентов для обслуживания
    double a, b; // Параметры распределения времени между клиентами

    std::vector<Agent> agents;
    std::priority_queue<Event, std::vector<Event>, std::greater<Event>> events;
    RandomGenerator rng;

    int clients_created;
    int clients_served;

public:
    System(int _n, int _m, double _a, double _b)
        : n(_n), m(_m), a(_a), b(_b), rng(_a, _b),
        clients_created(0), clients_served(0) {

        // Создаем агентов
        for (int i = 0; i < n; i++) {
            agents.emplace_back(i);
        }
    }

    // Запуск моделирования
    void run() {
        // Создаем первого клиента
        createNextClient(0.0);

        // Основной цикл событий
        while (!events.empty() && clients_served < m) {
            Event event = events.top();
            events.pop();

            if (event.type == 0) { // Прибытие клиента
                handleArrival(event);
            }
            else { // Завершение обслуживания
                handleDeparture(event);
            }
        }

        // Вывод результатов
        printReport();
    }

private:
    // Создать следующего клиента
    void createNextClient(double current_time) {
        if (clients_created >= m) {
            return;
        }

        double interval = (clients_created == 0) ? rng.getNextTime() : rng.getNextTime();
        double arrival_time = current_time + interval;
        int difficulty = rng.getDifficulty();

        Client client(clients_created + 1, arrival_time, difficulty);

        // Создаем событие прибытия
        events.push(Event(arrival_time, 0, client.id));

        clients_created++;

        // Создаем следующего клиента, если нужно
        if (clients_created < m) {
            createNextClient(arrival_time);
        }
    }

    // Обработка прибытия клиента
    void handleArrival(const Event& event) {
        if (clients_served >= m) {
            return; // Не принимаем новых клиентов после m-го
        }

        // Находим агента с минимальной загрузкой
        int selected_agent = 0;
        double min_load = agents[0].getCurrentLoad();

        for (int i = 1; i < n; i++) {
            double load = agents[i].getCurrentLoad();
            if (load < min_load || (load == min_load && i < selected_agent)) {
                min_load = load;
                selected_agent = i;
            }
        }

        // Создаем клиента
        double arrival_time = event.time;
        int difficulty = rng.getDifficulty();
        Client client(event.client_id, arrival_time, difficulty);

        // Добавляем клиента к выбранному агенту
        agents[selected_agent].addClient(client);

        // Если агент свободен, начинаем обслуживание
        if (agents[selected_agent].isFree(arrival_time)) {
            agents[selected_agent].startNextService(arrival_time, events);
        }
    }

    // Обработка завершения обслуживания
    void handleDeparture(const Event& event) {
        int agent_id = event.agent_id;

        // Завершаем текущее обслуживание
        agents[agent_id].finishService();
        clients_served++;

        // Если агент свободен и в его очереди есть клиенты, начинаем следующее обслуживание
        if (agents[agent_id].isFree(event.time) && agents[agent_id].getQueueSize() > 0) {
            agents[agent_id].startNextService(event.time, events);
        }
    }

    // Вывод отчета
    void printReport() {
        std::cout << "Отчет о работе агентов:" << std::endl;
        std::cout << "=======================" << std::endl;

        // Собираем статистику
        struct ReportEntry {
            int id;
            int served_count;
            double total_time;
        };

        std::vector<ReportEntry> report;
        for (const auto& agent : agents) {
            report.push_back({ agent.id, agent.served_count, agent.total_work_time });
        }

        // Сортируем по убыванию количества клиентов, при равенстве - по возрастанию времени
        std::sort(report.begin(), report.end(), [](const ReportEntry& a, const ReportEntry& b) {
            if (a.served_count != b.served_count) {
                return a.served_count > b.served_count;
            }
            return a.total_time < b.total_time;
            });

        // Выводим результаты
        std::cout << std::left << std::setw(10) << "ID агента"
            << std::setw(20) << "Клиентов обслужено"
            << std::setw(20) << "Время работы" << std::endl;
        std::cout << std::string(50, '-') << std::endl;

        for (const auto& entry : report) {
            std::cout << std::left << std::setw(10) << entry.id
                << std::setw(20) << entry.served_count
                << std::setw(20) << std::fixed << std::setprecision(2)
                << entry.total_time << std::endl;
        }

        std::cout << "\nВсего обслужено клиентов: " << clients_served << std::endl;
    }
};

int main() {
    setlocale(LC_ALL, "Russian");
    // Параметры системы
    int n = 3;    // Количество агентов
    int m = 10;   // Количество клиентов для обслуживания
    double a = 0.5; // Минимальное время между клиентами
    double b = 2.0; // Максимальное время между клиентами

    // Создание и запуск системы
    System system(n, m, a, b);
    system.run();

    return 0;
}