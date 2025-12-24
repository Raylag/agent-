import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
import numpy as np
from mpl_toolkits.mplot3d import Axes3D

# Загрузка данных
data = pd.read_csv('experiment_r_l.csv')

# Тепловая карта (исправленный синтаксис)
pivot_table = data.pivot(index='r', columns='l', values='win_probability')
plt.figure(figsize=(10, 8))
sns.heatmap(pivot_table, annot=True, fmt=".2f", cmap="YlOrRd")
plt.title("Вероятность победы агента (фиксировано n)")
plt.xlabel('l (максимальное перемещение)')
plt.ylabel('r (радиус болванчика)')
plt.show()

# 3D поверхность
fig = plt.figure(figsize=(12, 10))
ax = fig.add_subplot(111, projection='3d')

# Подготовка данных для 3D графика
X = pivot_table.columns.values.astype(float)
Y = pivot_table.index.values.astype(float)
X_grid, Y_grid = np.meshgrid(X, Y)
Z = pivot_table.values

# Построение поверхности
surf = ax.plot_surface(X_grid, Y_grid, Z, cmap='viridis', alpha=0.8)

# Настройки осей
ax.set_xlabel('l (максимальное перемещение)')
ax.set_ylabel('r (радиус болванчика)')
ax.set_zlabel('Вероятность победы')

# Цветовая шкала
fig.colorbar(surf, shrink=0.5, aspect=10)

plt.title("3D поверхность вероятности победы")
plt.tight_layout()
plt.show()

# Анализ других экспериментов
print("Анализ эксперимента 2 (r и n):")
data2 = pd.read_csv('experiment_r_n.csv')
if 'n' in data2.columns:
    # Для каждого r построим график зависимости от n
    for r_val in sorted(data2['r'].unique()):
        subset = data2[data2['r'] == r_val]
        plt.plot(subset['n'], subset['win_probability'], marker='o', label=f'r={r_val}')
    
    plt.xlabel('Количество квадратов (n)')
    plt.ylabel('Вероятность победы')
    plt.title('Зависимость вероятности победы от количества квадратов')
    plt.legend()
    plt.grid(True)
    plt.show()

print("Анализ эксперимента 3 (l и n):")
data3 = pd.read_csv('experiment_l_n.csv')
if 'n' in data3.columns:
    # Тепловая карта для l и n
    pivot_table3 = data3.pivot(index='l', columns='n', values='win_probability')
    plt.figure(figsize=(10, 8))
    sns.heatmap(pivot_table3, annot=True, fmt=".2f", cmap="Blues")
    plt.title("Вероятность победы агента (фиксировано r)")
    plt.xlabel('n (количество квадратов)')
    plt.ylabel('l (максимальное перемещение)')
    plt.show()

# Анализ оптимальных параметров
print("\nОптимальные параметры по каждому эксперименту:")
print("Эксперимент 1 (r и l):")
max_idx = data['win_probability'].idxmax()
print(f"  Максимальная вероятность: {data.loc[max_idx, 'win_probability']:.3f}")
print(f"  При r={data.loc[max_idx, 'r']}, l={data.loc[max_idx, 'l']}")
