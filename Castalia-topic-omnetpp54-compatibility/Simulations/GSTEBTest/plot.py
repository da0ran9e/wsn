import matplotlib.pyplot as plt
import networkx as nx
import re
import numpy as np
from math import sqrt

# --- CÁC THAM SỐ CẤU HÌNH ---
FILE_PATH = "nodetree.txt"
CELL_RADIUS = 80  # Bán kính này PHẢI khớp với cellRadius trong mô phỏng của bạn
MAX_X_BOUND = 250 # Giới hạn bản đồ để vẽ
MAX_Y_BOUND = 250

# --- Dữ liệu được đọc từ file ---
positions = {}
edges = []
is_sink = set()
cgw_nodes = set()
ngw_nodes = set()

# --- Bước 1: Đọc và phân tích file log ---
try:
    with open(FILE_PATH) as f:
        lines = [line.strip() for line in f if line.strip()]
except FileNotFoundError:
    print(f"Lỗi: Không tìm thấy file '{FILE_PATH}'. Vui lòng kiểm tra lại đường dẫn.")
    exit()

for line in lines:
    if "isSink = true" in line:
        m = re.search(r"SN\.node\[(\d+)\]", line)
        if m: is_sink.add(int(m.group(1)))
    elif "xCoor" in line or "yCoor" in line:
        m = re.search(r"SN\.node\[(\d+)\]\.(xCoor|yCoor) = ([\d.-]+)", line)
        if m:
            nid, coord, val = int(m.group(1)), m.group(2), float(m.group(3))
            if nid not in positions: positions[nid] = {'x': 0, 'y': 0}
            positions[nid][coord[0]] = val
    elif "CGW" in line and "NGW" in line:
        m = re.search(r"CGW (\d+) to NGW (\d+)", line)
        if m:
            cgw_nodes.add(int(m.group(1)))
            ngw_nodes.add(int(m.group(2)))
    elif "to" in line:
        m = re.match(r"(\d+)\s+to\s+(\d+)", line)
        if m:
            src, dst = int(m.group(1)), int(m.group(2))
            edges.append((src, dst))

pos_for_nx = {nid: (coords['x'], coords['y']) for nid, coords in positions.items()}
G = nx.DiGraph()
G.add_nodes_from(pos_for_nx)
G.add_edges_from(edges)

# --- Bước 2: Tô màu các node ---
node_colors = []
for node in G.nodes():
    if node in is_sink:
        node_colors.append("red")
    elif node in ngw_nodes:
        node_colors.append("blue")      # NGW màu xanh dương
    elif node in cgw_nodes:
        node_colors.append("green")
    else:
        node_colors.append("skyblue")

# --- Bước 3: Hàm vẽ lưới tổ ong CHUYÊN NGHIỆP ---
def axial_to_pixel(q, r, radius):
    """Chuyển đổi từ tọa độ Axial (q,r) sang tọa độ pixel (x,y)."""
    x = radius * (sqrt(3) * q + sqrt(3) / 2 * r)
    y = radius * (3. / 2. * r)
    return x, y

def draw_hex_grid(radius, max_x, max_y):
    """Vẽ lưới lục giác khớp nhau hoàn hảo, tâm (0,0) là ô đầu tiên."""
    # Xác định phạm vi các ô cần vẽ
    max_q = int(max_x / (radius * sqrt(3))) + 2
    max_r = int(max_y / (radius * 1.5)) + 2

    for r in range(-max_r, max_r):
        for q in range(-max_q, max_q):
            cx, cy = axial_to_pixel(q, r, radius)
            
            # Chỉ vẽ những ô nằm trong phạm vi bản đồ
            if abs(cx) > max_x + radius or abs(cy) > max_y + radius:
                continue

            hexagon = plt.Polygon([
                (cx + radius * np.cos(angle), cy + radius * np.sin(angle))
                for angle in np.linspace(np.pi/6, 2*np.pi + np.pi/6, 7)
            ], edgecolor='lightgray', facecolor='none', linestyle='--', linewidth=0.8)
            plt.gca().add_patch(hexagon)

# --- Bước 4: Vẽ đồ thị ---
plt.figure(figsize=(16, 16)) # Phóng to bản đồ
draw_hex_grid(radius=CELL_RADIUS, max_x=MAX_X_BOUND, max_y=MAX_Y_BOUND)

# Vẽ các thành phần mạng
nx.draw_networkx_nodes(G, pos_for_nx, node_color=node_colors, node_size=500, alpha=1.0)
nx.draw_networkx_labels(G, pos_for_nx, font_size=8, font_color='white', font_weight='bold')
nx.draw_networkx_edges(G, pos_for_nx, edge_color='black', arrows=True, width=1.0, alpha=0.7)

# Thiết lập hiển thị
plt.title("Node Map with Cellular Grid Overlay", fontsize=16)
plt.axis("equal")
plt.axis("off")
plt.tight_layout()
plt.show()