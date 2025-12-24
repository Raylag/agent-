#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <algorithm>
#include <fstream>
#include <string>

// Глобальный генератор случайных чисел
std::mt19937 rng(std::random_device{}());

// Структура для точки на корте
struct Point {
    double x, y;
    Point(double x = 0, double y = 0) : x(x), y(y) {}
};

// Структура для квадрата (клетки)
struct Square {
    int id;
    double x_min, x_max, y_min, y_max;
    Point center;
    Square(int id, double x_min, double x_max, double y_min, double y_max)
        : id(id), x_min(x_min), x_max(x_max), y_min(y_min), y_max(y_max),
        center((x_min + x_max) / 2, (y_min + y_max) / 2) {
    }
};

// Класс игрока (болванчика или агента)
class Player {
public:
    Point position;
    double radius;
    double max_move;
    bool is_agent;

    Player(double r, double l, bool agent = false)
        : radius(r), max_move(l), is_agent(agent) {
        // Начальная позиция в центре своей половины
        if (is_agent) {
            position = Point(0, 5); // Агент слева
        }
        else {
            position = Point(20, 5); // Болванчик справа
        }
    }

    // Может ли игрок отбить мяч в данной точке
    bool canReturn(const Point& ball) const {
        double dist = distance(position, ball);
        return dist <= radius;
    }

    // Перемещение к мячу (но не дальше max_move)
    void moveToBall(const Point& ball) {
        double dist = distance(position, ball);
        if (dist <= max_move) {
            position = ball;
        }
        else {
            double dx = ball.x - position.x;
            double dy = ball.y - position.y;
            position.x += (dx / dist) * max_move;
            position.y += (dy / dist) * max_move;
        }
    }

private:
    double distance(const Point& a, const Point& b) const {
        return std::sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
    }
};

// Класс для моделирования теннисного матча
class TennisSimulator {
private:
    // Параметры корта
    const double COURT_WIDTH = 20.0;
    const double COURT_HEIGHT = 10.0;
    const double AGENT_HALF = 0.0;    // x ∈ [0, 10]
    const double OPPONENT_HALF = 10.0; // x ∈ [10, 20]

    // Параметры игры
    double r;  // радиус действия болванчика
    double l;  // максимальное перемещение за удар
    int n;     // количество квадратов (должно быть квадратом целого числа)
    int grid_size; // размер сетки √n × √n

    // Игроки
    Player agent;
    Player opponent;

    // Сетка квадратов на половине корта болванчика
    std::vector<Square> squares;
    std::vector<std::vector<int>> square_grid; // для нахождения соседей

    // Счёт
    int agent_score;
    int opponent_score;

    // Генераторы случайных чисел
    std::uniform_real_distribution<double> uniform_dist;
    std::uniform_int_distribution<int> int_dist;

public:
    TennisSimulator(double r, double l, int n)
        : r(r), l(l), n(n),
        agent(2 * r, l, true),  // Агент имеет радиус 2r
        opponent(r, l, false),
        uniform_dist(0.0, 1.0),
        int_dist(0, 100) {

        // Проверяем, что n - квадрат целого числа
        grid_size = std::sqrt(n);
        if (grid_size * grid_size != n) {
            throw std::invalid_argument("n должно быть квадратом целого числа");
        }

        // Инициализация сетки квадратов
        initializeSquares();

        // Начальные позиции
        reset();
    }

    // Инициализация квадратов на половине корта болванчика
    void initializeSquares() {
        squares.clear();
        square_grid.assign(grid_size, std::vector<int>(grid_size, -1));

        double square_width = (COURT_WIDTH / 2) / grid_size;
        double square_height = COURT_HEIGHT / grid_size;

        int id = 0;
        for (int i = 0; i < grid_size; ++i) {
            for (int j = 0; j < grid_size; ++j) {
                double x_min = OPPONENT_HALF + i * square_width;
                double x_max = x_min + square_width;
                double y_min = j * square_height;
                double y_max = y_min + square_height;

                squares.emplace_back(id, x_min, x_max, y_min, y_max);
                square_grid[i][j] = id;
                ++id;
            }
        }
    }

    // Сброс состояния для нового розыгрыша
    void reset() {
        agent.position = Point(5, 5); // Центр левой половины
        opponent.position = Point(15, 5); // Центр правой половины
        agent_score = 0;
        opponent_score = 0;
    }

    // Алгоритм выбора квадрата для агента (стратегия)
    int chooseSquare() {
        // Стратегия: выбираем квадрат, наиболее удаленный от текущей позиции болванчика
        // чтобы у него было меньше шансов добраться до мяча

        int best_square = 0;
        double max_distance = -1;

        for (const auto& square : squares) {
            double dist = distance(square.center, opponent.position);

            // Учитываем, что болванчик может переместиться на расстояние l
            // Выбираем квадрат, до которого болванчику нужно бежать дольше всего
            if (dist > max_distance) {
                max_distance = dist;
                best_square = square.id;
            }
        }

        return best_square;
    }

    // Попадание мяча в квадрат с учетом погрешности
    Point hitBallWithError(int target_square) {
        double error_prob = uniform_dist(rng) * 100;

        // С вероятностью 5% попадаем в соседний квадрат или аут
        if (error_prob < 5) {
            // Находим координаты квадрата в сетке
            int i = -1, j = -1;
            for (int idx = 0; idx < squares.size(); ++idx) {
                if (squares[idx].id == target_square) {
                    i = idx / grid_size;
                    j = idx % grid_size;
                    break;
                }
            }

            // Список возможных направлений ошибки
            std::vector<std::pair<int, int>> directions = {
                {-1, 0}, {1, 0}, {0, -1}, {0, 1} // влево, вправо, вниз, вверх
            };

            // Фильтруем допустимые направления
            std::vector<std::pair<int, int>> valid_directions;
            for (const auto& dir : directions) {
                int new_i = i + dir.first;
                int new_j = j + dir.second;

                if (new_i >= 0 && new_i < grid_size && new_j >= 0 && new_j < grid_size) {
                    valid_directions.push_back(dir);
                }
            }

            // Если есть допустимые направления, выбираем случайное
            if (!valid_directions.empty()) {
                std::uniform_int_distribution<int> dir_dist(0, valid_directions.size() - 1);
                auto dir = valid_directions[dir_dist(rng)];
                int new_i = i + dir.first;
                int new_j = j + dir.second;

                // Возвращаем случайную точку в новом квадрате
                const auto& new_square = squares[square_grid[new_i][new_j]];
                return randomPointInSquare(new_square);
            }
            else {
                // Если все направления ведут за пределы корта - аут
                return Point(-1, -1); // Помечаем как аут
            }
        }

        // С вероятностью 95% попадаем в выбранный квадрат
        const auto& square = squares[target_square];
        return randomPointInSquare(square);
    }

    // Проверка, находится ли точка в пределах корта
    bool isInCourt(const Point& p) const {
        return p.x >= 0 && p.x <= COURT_WIDTH && p.y >= 0 && p.y <= COURT_HEIGHT;
    }

    // Проверка, находится ли точка в половине корта противника
    bool isInOpponentHalf(const Point& p) const {
        return p.x >= OPPONENT_HALF && p.x <= COURT_WIDTH && p.y >= 0 && p.y <= COURT_HEIGHT;
    }

    // Моделирование одного розыгрыша
    bool simulateRally() {
        // Болванчик подает (случайная точка в половине агента)
        Point ball = randomPointInAgentHalf();

        while (true) {
            // Агент пытается отбить
            if (agent.canReturn(ball)) {
                agent.moveToBall(ball);

                // Агент выбирает квадрат и бьет
                int target_square = chooseSquare();
                ball = hitBallWithError(target_square);

                // Проверка на аут
                if (!isInCourt(ball) || !isInOpponentHalf(ball)) {
                    return false; // Агент проиграл розыгрыш
                }
            }
            else {
                return false; // Агент не отбил
            }

            // Болванчик пытается отбить
            if (opponent.canReturn(ball)) {
                opponent.moveToBall(ball);

                // Болванчик бьет в случайную точку половины агента
                ball = randomPointInAgentHalf();
            }
            else {
                return true; // Болванчик не отбил, агент выиграл
            }
        }
    }

    // Моделирование одного гейма (до 4 очков с разницей в 2)
    bool simulateGame() {
        int points_to_win = 4;
        int agent_points = 0, opponent_points = 0;

        while (true) {
            if (simulateRally()) {
                ++agent_points;
            }
            else {
                ++opponent_points;
            }

            // Проверка условий победы в гейме
            if (agent_points >= points_to_win && agent_points - opponent_points >= 2) {
                return true;
            }
            if (opponent_points >= points_to_win && opponent_points - agent_points >= 2) {
                return false;
            }

            // Сброс позиций для следующего розыгрыша
            agent.position = Point(5, 5);
            opponent.position = Point(15, 5);
        }
    }

    // Моделирование матча (до 2 выигранных сетов)
    bool simulateMatch() {
        int agent_sets = 0, opponent_sets = 0;

        while (agent_sets < 2 && opponent_sets < 2) {
            if (simulateGame()) {
                ++agent_sets;
            }
            else {
                ++opponent_sets;
            }

            // Логирование счета (опционально)
            // std::cout << "Счет: Агент " << agent_sets << " - " << opponent_sets << " Болванчик\n";
        }

        return agent_sets == 2;
    }

    // Запуск множества матчей для оценки вероятности победы
    double estimateWinProbability(int num_matches = 1000) {
        int wins = 0;

        for (int i = 0; i < num_matches; ++i) {
            reset();
            if (simulateMatch()) {
                ++wins;
            }

            // Прогресс (опционально)
            if ((i + 1) % 100 == 0) {
                // std::cout << "Сыграно матчей: " << i + 1 << std::endl;
            }
        }

        return static_cast<double>(wins) / num_matches;
    }

private:
    // Вспомогательные функции
    double distance(const Point& a, const Point& b) const {
        return std::sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
    }

    Point randomPointInSquare(const Square& square) {
        std::uniform_real_distribution<double> x_dist(square.x_min, square.x_max);
        std::uniform_real_distribution<double> y_dist(square.y_min, square.y_max);
        return Point(x_dist(rng), y_dist(rng));
    }

    Point randomPointInAgentHalf() {
        std::uniform_real_distribution<double> x_dist(0.0, OPPONENT_HALF);
        std::uniform_real_distribution<double> y_dist(0.0, COURT_HEIGHT);
        return Point(x_dist(rng), y_dist(rng));
    }
};

// Функция для проведения экспериментов и записи результатов
void runExperiments() {
    // Параметры для экспериментов
    std::vector<double> r_values = { 0.5, 1.0, 1.5, 2.0, 2.5 };
    std::vector<double> l_values = { 0.5, 1.0, 1.5, 2.0, 2.5 };
    std::vector<int> n_values = { 4, 9, 16, 25, 36 }; // Квадраты чисел

    // Фиксируем один параметр, меняем два других
    double fixed_r = 1.5;
    double fixed_l = 1.0;
    int fixed_n = 16;

    // Эксперимент 1: меняем r и l, фиксируем n
    std::ofstream file1("experiment_r_l.csv");
    file1 << "r,l,win_probability\n";

    std::cout << "Эксперимент 1: меняем r и l (n = " << fixed_n << ")\n";
    for (double r : r_values) {
        for (double l : l_values) {
            TennisSimulator simulator(r, l, fixed_n);
            double win_prob = simulator.estimateWinProbability(500);
            file1 << r << "," << l << "," << win_prob << "\n";
            std::cout << "r=" << r << ", l=" << l << ", win_prob=" << win_prob << std::endl;
        }
    }
    file1.close();

    // Эксперимент 2: меняем r и n, фиксируем l
    std::ofstream file2("experiment_r_n.csv");
    file2 << "r,n,win_probability\n";

    std::cout << "\nЭксперимент 2: меняем r и n (l = " << fixed_l << ")\n";
    for (double r : r_values) {
        for (int n : n_values) {
            try {
                TennisSimulator simulator(r, fixed_l, n);
                double win_prob = simulator.estimateWinProbability(500);
                file2 << r << "," << n << "," << win_prob << "\n";
                std::cout << "r=" << r << ", n=" << n << ", win_prob=" << win_prob << std::endl;
            }
            catch (const std::exception& e) {
                std::cerr << "Ошибка для n=" << n << ": " << e.what() << std::endl;
            }
        }
    }
    file2.close();

    // Эксперимент 3: меняем l и n, фиксируем r
    std::ofstream file3("experiment_l_n.csv");
    file3 << "l,n,win_probability\n";

    std::cout << "\nЭксперимент 3: меняем l и n (r = " << fixed_r << ")\n";
    for (double l : l_values) {
        for (int n : n_values) {
            try {
                TennisSimulator simulator(fixed_r, l, n);
                double win_prob = simulator.estimateWinProbability(500);
                file3 << l << "," << n << "," << win_prob << "\n";
                std::cout << "l=" << l << ", n=" << n << ", win_prob=" << win_prob << std::endl;
            }
            catch (const std::exception& e) {
                std::cerr << "Ошибка для n=" << n << ": " << e.what() << std::endl;
            }
        }
    }
    file3.close();

    std::cout << "\nЭксперименты завершены. Данные сохранены в файлы CSV.\n";
    std::cout << "Для построения графиков можно использовать следующие команды Python:\n";
    std::cout << "1. Загрузить данные: data = pd.read_csv('experiment_r_l.csv')\n";
    std::cout << "2. Построить тепловую карту: sns.heatmap(data.pivot('r', 'l', 'win_probability'))\n";
    std::cout << "3. Или 3D график: fig = plt.figure(); ax = fig.add_subplot(111, projection='3d')\n";
}

int main() {
    setlocale(LC_ALL, "Russian");
    try {
        // Пример одиночного запуска
        TennisSimulator simulator(1.5, 1.0, 16);
        std::cout << "Тестовый запуск...\n";
        double win_prob = simulator.estimateWinProbability(100);
        std::cout << "Вероятность победы агента: " << win_prob * 100 << "%\n";

        // Запуск полного эксперимента
        char run_experiments;
        std::cout << "\nЗапустить полный эксперимент? (y/n): ";
        std::cin >> run_experiments;

        if (run_experiments == 'y' || run_experiments == 'Y') {
            runExperiments();
        }

    }
    catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}