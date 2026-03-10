const viewport = document.getElementById('mapViewport');
const content = document.getElementById('mapContent');
const gridLayer = document.getElementById('gridLayer');
const hexLayer = document.getElementById('hexLayer');
const hexColorLayer = document.getElementById('hexColorLayer');
const suspiciousCellLayer = document.getElementById('suspiciousCellLayer');
const nodeLayer = document.getElementById('nodeLayer');
const nodeColorLayer = document.getElementById('nodeColorLayer');
const suspiciousNodeLayer = document.getElementById('suspiciousNodeLayer');
const uavPathLayer = document.getElementById('uavPathLayer');
const uavEventLayer = document.getElementById('uavEventLayer');
const communicationLayer = document.getElementById('communicationLayer');
const resultFileInput = document.getElementById('resultFileInput');
const toggleGrid = document.getElementById('toggleGrid');
const toggleHex = document.getElementById('toggleHex');
const toggleHexColor = document.getElementById('toggleHexColor');
const toggleNode = document.getElementById('toggleNode');
const toggleNodeColor = document.getElementById('toggleNodeColor');
const toggleSuspiciousNode = document.getElementById('toggleSuspiciousNode');
const toggleUavPath = document.getElementById('toggleUavPath');
const toggleCommunication = document.getElementById('toggleCommunication');
const nodeStats = document.getElementById('nodeStats');
const playPauseBtn = document.getElementById('playPauseBtn');
const prevFrameBtn = document.getElementById('prevFrameBtn');
const nextFrameBtn = document.getElementById('nextFrameBtn');
const eventSlider = document.getElementById('eventSlider');
const eventFrameLabel = document.getElementById('eventFrameLabel');
const eventTimeLabel = document.getElementById('eventTimeLabel');
const zoomInBtn = document.getElementById('zoomInBtn');
const zoomOutBtn = document.getElementById('zoomOutBtn');
const resetViewBtn = document.getElementById('resetViewBtn');

const MAP_WIDTH = 2400;
const MAP_HEIGHT = 2400;
const DEFAULT_CELL_RADIUS_M = 80;
const WORLD_PADDING_M = 80;
const HEX_GRID_OFFSET_BASE = 10000;
const HEX_GRID_OFFSET_MULTIPLIER = 100;
const HEX_GRID_OFFSET_EXTRA = 1000;
const NODE_RADIUS = 10;
const UAV_EVENT_SCALE = 3;

const state = {
  scale: 1,
  minScale: 0.3,
  maxScale: 4,
  x: 0,
  y: 0,
  isDragging: false,
  dragStartX: 0,
  dragStartY: 0,
};

const playback = {
  events: [],
  waypointEvents: [],
  communicationLinks: [],
  frameIndex: 0,
  isPlaying: false,
  timer: null,
  frameIntervalMs: 550,
  world: null,
  uavColorMap: new Map(),
};

function applyTransform() {
  content.style.transform = `translate(${state.x}px, ${state.y}px) scale(${state.scale})`;
}

function clampScale(nextScale) {
  return Math.max(state.minScale, Math.min(state.maxScale, nextScale));
}

function zoomAt(clientX, clientY, zoomFactor) {
  const rect = viewport.getBoundingClientRect();
  const px = clientX - rect.left;
  const py = clientY - rect.top;

  const prevScale = state.scale;
  const nextScale = clampScale(prevScale * zoomFactor);
  if (nextScale === prevScale) {
    return;
  }

  const worldX = (px - state.x) / prevScale;
  const worldY = (py - state.y) / prevScale;

  state.scale = nextScale;
  state.x = px - worldX * nextScale;
  state.y = py - worldY * nextScale;

  applyTransform();
}

function resetView() {
  const rect = viewport.getBoundingClientRect();
  state.scale = 0.45;
  state.x = (rect.width - MAP_WIDTH * state.scale) / 2;
  state.y = (rect.height - MAP_HEIGHT * state.scale) / 2;
  applyTransform();
}

function buildHexPath(cx, cy, r) {
  const points = [];
  for (let i = 0; i < 6; i++) {
    const angle = (-90 + i * 60) * (Math.PI / 180);
    const x = cx + r * Math.cos(angle);
    const y = cy + r * Math.sin(angle);
    points.push(`${x.toFixed(2)},${y.toFixed(2)}`);
  }
  return points.join(' ');
}

function parseConfig(text) {
  const config = {
    cellRadius: DEFAULT_CELL_RADIUS_M,
  };

  const radiusMatch = text.match(/^\s*Cell Radius:\s*([\d.]+)m\s*$/im);
  if (radiusMatch) {
    config.cellRadius = Number(radiusMatch[1]);
  }

  return config;
}

function buildWorldMapper(nodes) {
  if (!nodes.length) {
    return {
      minX: 0,
      minY: 0,
      maxX: 1,
      maxY: 1,
      scale: 1,
      offsetX: 0,
      offsetY: 0,
      toMapPoint(x, y) {
        return { x, y };
      },
    };
  }

  let minX = Number.POSITIVE_INFINITY;
  let minY = Number.POSITIVE_INFINITY;
  let maxX = Number.NEGATIVE_INFINITY;
  let maxY = Number.NEGATIVE_INFINITY;

  for (const n of nodes) {
    minX = Math.min(minX, n.x);
    minY = Math.min(minY, n.y);
    maxX = Math.max(maxX, n.x);
    maxY = Math.max(maxY, n.y);
  }

  const worldWidth = Math.max(1, (maxX - minX) + WORLD_PADDING_M * 2);
  const worldHeight = Math.max(1, (maxY - minY) + WORLD_PADDING_M * 2);

  const usableWidth = MAP_WIDTH - 260;
  const usableHeight = MAP_HEIGHT - 260;
  const scale = Math.min(usableWidth / worldWidth, usableHeight / worldHeight);

  const offsetX = (MAP_WIDTH - worldWidth * scale) / 2;
  const offsetY = (MAP_HEIGHT - worldHeight * scale) / 2;

  return {
    minX,
    minY,
    maxX,
    maxY,
    scale,
    offsetX,
    offsetY,
    toMapPoint(x, y) {
      return {
        x: offsetX + (x - (minX - WORLD_PADDING_M)) * scale,
        y: offsetY + (y - (minY - WORLD_PADDING_M)) * scale,
      };
    },
  };
}

function computeDefaultHexGridOffset(nodeCount) {
  const gridSizeApprox = Math.round(Math.sqrt(Math.max(1, nodeCount)));
  return Math.max(
    HEX_GRID_OFFSET_BASE,
    gridSizeApprox * HEX_GRID_OFFSET_MULTIPLIER + HEX_GRID_OFFSET_EXTRA,
  );
}

function decodeCellAxial(cellId, gridOffset) {
  const r = Math.round(cellId / gridOffset);
  const q = cellId - r * gridOffset;
  return { q, r };
}

function computeHexCellCenterFromAxial(q, r, cellRadius) {
  const sqrt3 = Math.sqrt(3);
  return {
    centerX: cellRadius * (sqrt3 * q + (sqrt3 / 2) * r),
    centerY: cellRadius * (1.5 * r),
  };
}

function buildCellInfo(nodes, cellRadiusMeters) {
  const cellMap = new Map();
  const gridOffset = computeDefaultHexGridOffset(nodes.length);

  for (const node of nodes) {
    if (!cellMap.has(node.cellId)) {
      const axial = decodeCellAxial(node.cellId, gridOffset);
      const center = computeHexCellCenterFromAxial(axial.q, axial.r, cellRadiusMeters);
      cellMap.set(node.cellId, {
        cellId: node.cellId,
        q: axial.q,
        r: axial.r,
        colorId: node.colorId,
        centerX: center.centerX,
        centerY: center.centerY,
      });
    }
  }

  return Array.from(cellMap.values());
}

function renderHexLayer(world, cellRadiusMeters, cells) {
  const hexRadius = Math.max(1, cellRadiusMeters * world.scale);

  hexLayer.setAttribute('viewBox', `0 0 ${MAP_WIDTH} ${MAP_HEIGHT}`);
  hexLayer.setAttribute('width', `${MAP_WIDTH}`);
  hexLayer.setAttribute('height', `${MAP_HEIGHT}`);
  hexLayer.innerHTML = '';

  hexColorLayer.setAttribute('viewBox', `0 0 ${MAP_WIDTH} ${MAP_HEIGHT}`);
  hexColorLayer.setAttribute('width', `${MAP_WIDTH}`);
  hexColorLayer.setAttribute('height', `${MAP_HEIGHT}`);
  hexColorLayer.innerHTML = '';

  for (const cell of cells) {
    const center = world.toMapPoint(cell.centerX, cell.centerY);
    const points = buildHexPath(center.x, center.y, hexRadius);
    const colorIndex = ((cell.q - cell.r) % 3 + 3) % 3;

    const fillPoly = document.createElementNS('http://www.w3.org/2000/svg', 'polygon');
    fillPoly.setAttribute('points', points);
    fillPoly.setAttribute('class', `hex-color-cell hex-color-${colorIndex}`);
    hexColorLayer.appendChild(fillPoly);

    const polygon = document.createElementNS('http://www.w3.org/2000/svg', 'polygon');
    polygon.setAttribute('points', points);
    polygon.setAttribute('class', 'hex-cell');
    hexLayer.appendChild(polygon);
  }
}

function renderSuspiciousCellLayer(world, cellRadiusMeters, cells, suspiciousCellSet) {
  const hexRadius = Math.max(1, cellRadiusMeters * world.scale);

  suspiciousCellLayer.setAttribute('viewBox', `0 0 ${MAP_WIDTH} ${MAP_HEIGHT}`);
  suspiciousCellLayer.setAttribute('width', `${MAP_WIDTH}`);
  suspiciousCellLayer.setAttribute('height', `${MAP_HEIGHT}`);
  suspiciousCellLayer.innerHTML = '';

  for (const cell of cells) {
    if (!suspiciousCellSet.has(cell.cellId)) {
      continue;
    }

    const center = world.toMapPoint(cell.centerX, cell.centerY);
    const points = buildHexPath(center.x, center.y, hexRadius);

    const polygon = document.createElementNS('http://www.w3.org/2000/svg', 'polygon');
    polygon.setAttribute('points', points);
    polygon.setAttribute('class', 'suspicious-cell');
    suspiciousCellLayer.appendChild(polygon);
  }
}

function parseNodeInfo(text) {
  const result = [];
  const lines = text.split(/\r?\n/);
  const regex = /^\[NODE-INFO\]\s+(\d+)\s+(-?\d+)\s+(\d+)\s+([\d.]+)\s+([\d.]+)/;

  for (const line of lines) {
    const m = line.match(regex);
    if (!m) {
      continue;
    }
    result.push({
      nodeId: Number(m[1]),
      cellId: Number(m[2]),
      colorId: Number(m[3]),
      x: Number(m[4]),
      y: Number(m[5]),
    });
  }
  return result;
}

function parseCellLeaders(text) {
  const leaders = new Set();
  const lines = text.split(/\r?\n/);
  const regex = /^\[CELL-LEADER\]\s+\[-?\d+\]\s+(\d+)\s+\([\d.]+\)/;

  for (const line of lines) {
    const m = line.match(regex);
    if (!m) {
      continue;
    }
    leaders.add(Number(m[1]));
  }

  return leaders;
}

function parseSuspiciousInfo(text) {
  const suspiciousNodes = new Set();
  const suspiciousCells = new Set();
  let suspiciousPoint = null;

  const lines = text.split(/\r?\n/);

  // Parse the actual seed point line: [SUSPICIOUS-POINT] x y
  for (const line of lines) {
    if (!line.startsWith('[SUSPICIOUS-POINT]')) {
      continue;
    }
    if (line.includes('seedPos.x')) {
      continue; // skip header line
    }
    const nums = line.match(/-?\d+(?:\.\d+)?/g);
    if (nums && nums.length >= 2) {
      suspiciousPoint = {
        x: Number(nums[0]),
        y: Number(nums[1]),
      };
      break;
    }
  }

  // Parse the actual region line, skip descriptor header line.
  for (const line of lines) {
    if (!line.startsWith('[SUSPICIOUS-REGION]')) {
      continue;
    }
    if (line.includes('nodeId1')) {
      continue; // skip header line
    }

    const payload = line.replace('[SUSPICIOUS-REGION]', '').trim();
    const parts = payload.split(/\(Cells?:/i);
    const nodePart = parts[0] || '';
    const cellPart = parts[1] || '';

    const nodeNums = nodePart.match(/\d+/g);
    if (nodeNums) {
      for (const n of nodeNums) {
        suspiciousNodes.add(Number(n));
      }
    }

    const cellNums = cellPart.match(/\d+/g);
    if (cellNums) {
      for (const c of cellNums) {
        suspiciousCells.add(Number(c));
      }
    }
  }

  // Fallback if region line not parsed
  if (suspiciousNodes.size === 0) {
    const nodeIdsMatch = text.match(/^\s*Node IDs:\s*(.+)$/m);
    if (nodeIdsMatch) {
      const nums = nodeIdsMatch[1].match(/\d+/g);
      if (nums) {
        for (const n of nums) {
          suspiciousNodes.add(Number(n));
        }
      }
    }
  }

  return {
    suspiciousNodes,
    suspiciousCells,
    suspiciousPoint,
  };
}

function parseUavPaths(text) {
  const lines = text.split(/\r?\n/);
  const paths = [];

  for (const line of lines) {
    if (!line.startsWith('[UAV-PATH]')) {
      continue;
    }
    if (line.includes('uav2NodeId') || !line.includes('waypoints:')) {
      continue;
    }

    const header = line.match(/^\[UAV-PATH\]\s+(\d+)\b/);
    if (!header) {
      continue;
    }

    const uavId = Number(header[1]);
    const waypointsPart = line.split('waypoints:')[1] || '';
    const waypointRegex = /\((-?\d+(?:\.\d+)?),\s*(-?\d+(?:\.\d+)?)\)/g;
    const waypoints = [];

    let match;
    while ((match = waypointRegex.exec(waypointsPart)) !== null) {
      waypoints.push({ x: Number(match[1]), y: Number(match[2]) });
    }

    if (waypoints.length > 0) {
      paths.push({ uavId, waypoints });
    }
  }

  paths.sort((a, b) => a.uavId - b.uavId);
  return paths;
}

function parseUavWaypointEvents(text) {
  const lines = text.split(/\r?\n/);
  const events = [];
  const regex = /^\[EVENT\]\s*([\d.]+)\s*\|\s*event=UAVWaypointArrival\s*\|\s*nodeId=(\d+)\s*\|\s*pos=\((-?[\d.]+),(-?[\d.]+),(-?[\d.]+)\)/;

  for (const line of lines) {
    const m = line.match(regex);
    if (!m) {
      continue;
    }

    events.push({
      time: Number(m[1]),
      nodeId: Number(m[2]),
      x: Number(m[3]),
      y: Number(m[4]),
      z: Number(m[5]),
    });
  }

  events.sort((a, b) => a.time - b.time);
  return events;
}

function parseCommunicationLinks(text) {
  const lines = text.split(/\r?\n/);
  const links = [];
  const seen = new Set();
  let currentEventTime = null;

  const eventRegex = /^\[EVENT\]\s*([\d.]+)\s*\|/;
  const tokenRegex = /(\d+)-(R|S)-(\d+)\((\d+)\)/g;

  for (const line of lines) {
    const eventMatch = line.match(eventRegex);
    if (eventMatch) {
      currentEventTime = Number(eventMatch[1]);
      continue;
    }

    if (currentEventTime === null) {
      continue;
    }

    let tokenMatch;
    while ((tokenMatch = tokenRegex.exec(line)) !== null) {
      const srcId = Number(tokenMatch[1]);
      const linkType = tokenMatch[2];
      const dstId = Number(tokenMatch[3]);

      const dedupeKey = `${currentEventTime}|${srcId}|${linkType}|${dstId}`;
      if (seen.has(dedupeKey)) {
        continue;
      }
      seen.add(dedupeKey);

      links.push({
        time: currentEventTime,
        srcId,
        dstId,
        linkType,
      });
    }
  }

  links.sort((a, b) => a.time - b.time);
  return links;
}

function buildUavEventTracks(waypointEvents) {
  const tracks = new Map();
  for (const ev of waypointEvents) {
    if (!tracks.has(ev.nodeId)) {
      tracks.set(ev.nodeId, []);
    }
    tracks.get(ev.nodeId).push(ev);
  }
  return tracks;
}

function getNodePositionAtTime(nodeId, time, groundNodeMap, uavTracks) {
  const groundNode = groundNodeMap.get(nodeId);
  if (groundNode) {
    return { x: groundNode.x, y: groundNode.y };
  }

  const track = uavTracks.get(nodeId);
  if (!track || track.length === 0) {
    return null;
  }

  let latest = null;
  for (const ev of track) {
    if (ev.time <= time) {
      latest = ev;
    } else {
      break;
    }
  }

  if (!latest) {
    return null;
  }

  return { x: latest.x, y: latest.y };
}

function resolveCommunicationGeometry(communicationLinks, nodes, waypointEvents) {
  const groundNodeMap = new Map(nodes.map((n) => [n.nodeId, n]));
  const uavTracks = buildUavEventTracks(waypointEvents);
  const resolved = [];

  for (const link of communicationLinks) {
    const srcPos = getNodePositionAtTime(link.srcId, link.time, groundNodeMap, uavTracks);
    const dstPos = getNodePositionAtTime(link.dstId, link.time, groundNodeMap, uavTracks);
    if (!srcPos || !dstPos) {
      continue;
    }

    resolved.push({
      ...link,
      srcX: srcPos.x,
      srcY: srcPos.y,
      dstX: dstPos.x,
      dstY: dstPos.y,
    });
  }

  return resolved;
}

function buildPlaybackFrames(waypointEvents, communicationLinks) {
  const timeSet = new Set();

  for (const ev of waypointEvents) {
    timeSet.add(ev.time);
  }

  for (const link of communicationLinks) {
    timeSet.add(link.time);
  }

  return Array.from(timeSet)
    .sort((a, b) => a - b)
    .map((time) => ({ time }));
}

function renderNodeLayer(nodes, world) {
  nodeLayer.setAttribute('viewBox', `0 0 ${MAP_WIDTH} ${MAP_HEIGHT}`);
  nodeLayer.setAttribute('width', `${MAP_WIDTH}`);
  nodeLayer.setAttribute('height', `${MAP_HEIGHT}`);
  nodeLayer.innerHTML = '';

  for (const node of nodes) {
    const p = world.toMapPoint(node.x, node.y);

    const dot = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
    dot.setAttribute('cx', p.x);
    dot.setAttribute('cy', p.y);
    dot.setAttribute('r', NODE_RADIUS);
    dot.setAttribute('class', 'node-dot');
    nodeLayer.appendChild(dot);

    const label = document.createElementNS('http://www.w3.org/2000/svg', 'text');
    label.setAttribute('x', p.x + 8);
    label.setAttribute('y', p.y + 3);
    label.setAttribute('class', 'node-label');
    label.textContent = `${node.nodeId}`;
    nodeLayer.appendChild(label);
  }

  nodeStats.textContent = `Nodes: ${nodes.length}`;
}

function renderNodeColorLayer(nodes, world, leaderSet) {
  nodeColorLayer.setAttribute('viewBox', `0 0 ${MAP_WIDTH} ${MAP_HEIGHT}`);
  nodeColorLayer.setAttribute('width', `${MAP_WIDTH}`);
  nodeColorLayer.setAttribute('height', `${MAP_HEIGHT}`);
  nodeColorLayer.innerHTML = '';

  for (const node of nodes) {
    const p = world.toMapPoint(node.x, node.y);

    const dot = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
    dot.setAttribute('cx', p.x);
    dot.setAttribute('cy', p.y);
    dot.setAttribute('r', NODE_RADIUS - 1);

    if (leaderSet.has(node.nodeId)) {
      dot.setAttribute('class', 'node-color-dot node-color-cl');
    } else {
      const colorIndex = ((node.colorId % 3) + 3) % 3;
      dot.setAttribute('class', `node-color-dot node-color-${colorIndex}`);
    }

    nodeColorLayer.appendChild(dot);
  }
}

function renderSuspiciousNodeLayer(nodes, world, suspiciousSet, suspiciousPoint) {
  suspiciousNodeLayer.setAttribute('viewBox', `0 0 ${MAP_WIDTH} ${MAP_HEIGHT}`);
  suspiciousNodeLayer.setAttribute('width', `${MAP_WIDTH}`);
  suspiciousNodeLayer.setAttribute('height', `${MAP_HEIGHT}`);
  suspiciousNodeLayer.innerHTML = '';

  if (suspiciousPoint) {
    const sp = world.toMapPoint(suspiciousPoint.x, suspiciousPoint.y);

    const pointDot = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
    pointDot.setAttribute('cx', sp.x);
    pointDot.setAttribute('cy', sp.y);
    pointDot.setAttribute('r', 8);
    pointDot.setAttribute('class', 'suspicious-point-dot');
    suspiciousNodeLayer.appendChild(pointDot);

    const pointLabel = document.createElementNS('http://www.w3.org/2000/svg', 'text');
    pointLabel.setAttribute('x', sp.x + 10);
    pointLabel.setAttribute('y', sp.y - 10);
    pointLabel.setAttribute('class', 'suspicious-point-label');
    pointLabel.textContent = 'Suspicious Point';
    suspiciousNodeLayer.appendChild(pointLabel);
  }

  for (const node of nodes) {
    if (!suspiciousSet.has(node.nodeId)) {
      continue;
    }

    const p = world.toMapPoint(node.x, node.y);

    const dot = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
    dot.setAttribute('cx', p.x);
    dot.setAttribute('cy', p.y);
    dot.setAttribute('r', NODE_RADIUS);
    dot.setAttribute('class', 'suspicious-node-dot');
    suspiciousNodeLayer.appendChild(dot);

    const label = document.createElementNS('http://www.w3.org/2000/svg', 'text');
    label.setAttribute('x', p.x + 8);
    label.setAttribute('y', p.y + 3);
    label.setAttribute('class', 'suspicious-node-label');
    label.textContent = `${node.nodeId}`;
    suspiciousNodeLayer.appendChild(label);
  }
}

function renderUavPathLayer(world, uavPaths) {
  uavPathLayer.setAttribute('viewBox', `0 0 ${MAP_WIDTH} ${MAP_HEIGHT}`);
  uavPathLayer.setAttribute('width', `${MAP_WIDTH}`);
  uavPathLayer.setAttribute('height', `${MAP_HEIGHT}`);
  uavPathLayer.innerHTML = '';

  for (let i = 0; i < uavPaths.length; i++) {
    const path = uavPaths[i];
    const colorIndex = i % 2; // 0: xanh lá, 1: xanh lam
    const points = path.waypoints.map((wp) => world.toMapPoint(wp.x, wp.y));

    const polyline = document.createElementNS('http://www.w3.org/2000/svg', 'polyline');
    polyline.setAttribute('points', points.map((p) => `${p.x.toFixed(2)},${p.y.toFixed(2)}`).join(' '));
    polyline.setAttribute('class', `uav-path uav-path-${colorIndex}`);
    uavPathLayer.appendChild(polyline);

    for (const p of points) {
      const dot = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
      dot.setAttribute('cx', p.x);
      dot.setAttribute('cy', p.y);
      dot.setAttribute('r', 3.5);
      dot.setAttribute('class', `uav-waypoint uav-waypoint-${colorIndex}`);
      uavPathLayer.appendChild(dot);
    }

    if (points.length > 0) {
      const first = points[0];
      const label = document.createElementNS('http://www.w3.org/2000/svg', 'text');
      label.setAttribute('x', first.x + 6);
      label.setAttribute('y', first.y - 6);
      label.setAttribute('class', `uav-label uav-label-${colorIndex}`);
      label.textContent = `UAV ${path.uavId}`;
      uavPathLayer.appendChild(label);
    }
  }
}

function buildUavColorMap(uavPaths, waypointEvents) {
  const colorMap = new Map();

  for (let i = 0; i < uavPaths.length; i++) {
    colorMap.set(uavPaths[i].uavId, i % 2);
  }

  if (colorMap.size === 0) {
    const ids = [...new Set(waypointEvents.map((e) => e.nodeId))].sort((a, b) => a - b);
    for (let i = 0; i < ids.length; i++) {
      colorMap.set(ids[i], i % 2);
    }
  }

  return colorMap;
}

function renderUavEventFrame() {
  const world = playback.world;
  const frames = playback.events;
  const waypointEvents = playback.waypointEvents;

  uavEventLayer.setAttribute('viewBox', `0 0 ${MAP_WIDTH} ${MAP_HEIGHT}`);
  uavEventLayer.setAttribute('width', `${MAP_WIDTH}`);
  uavEventLayer.setAttribute('height', `${MAP_HEIGHT}`);
  uavEventLayer.innerHTML = '';

  if (!world || frames.length === 0) {
    eventFrameLabel.textContent = 'Frame: 0/0';
    eventTimeLabel.textContent = 'Time: --';
    return;
  }

  const clampedIndex = Math.max(0, Math.min(playback.frameIndex, frames.length - 1));
  playback.frameIndex = clampedIndex;
  const currentTime = frames[clampedIndex].time;
  const latestByUav = new Map();

  for (const wp of waypointEvents) {
    if (wp.time <= currentTime) {
      latestByUav.set(wp.nodeId, wp);
    } else {
      break;
    }
  }

  for (const [uavId, ev] of latestByUav) {
    const p = world.toMapPoint(ev.x, ev.y);
    const colorIndex = playback.uavColorMap.get(uavId) ?? 0;
    const size = NODE_RADIUS * UAV_EVENT_SCALE;

    const h = document.createElementNS('http://www.w3.org/2000/svg', 'line');
    h.setAttribute('x1', p.x - size);
    h.setAttribute('y1', p.y);
    h.setAttribute('x2', p.x + size);
    h.setAttribute('y2', p.y);
    h.setAttribute('class', `uav-event-plus uav-event-plus-${colorIndex}`);
    h.setAttribute('stroke-width', String(Math.max(3, NODE_RADIUS * 0.5)));
    uavEventLayer.appendChild(h);

    const v = document.createElementNS('http://www.w3.org/2000/svg', 'line');
    v.setAttribute('x1', p.x);
    v.setAttribute('y1', p.y - size);
    v.setAttribute('x2', p.x);
    v.setAttribute('y2', p.y + size);
    v.setAttribute('class', `uav-event-plus uav-event-plus-${colorIndex}`);
    v.setAttribute('stroke-width', String(Math.max(3, NODE_RADIUS * 0.5)));
    uavEventLayer.appendChild(v);

    const label = document.createElementNS('http://www.w3.org/2000/svg', 'text');
    label.setAttribute('x', p.x + 12);
    label.setAttribute('y', p.y - 12);
    label.setAttribute('class', `uav-event-label uav-event-label-${colorIndex}`);
    label.textContent = `UAV ${uavId}`;
    uavEventLayer.appendChild(label);
  }

  eventFrameLabel.textContent = `Frame: ${clampedIndex + 1}/${frames.length}`;
  eventTimeLabel.textContent = `Time: ${currentTime.toFixed(3)}s`;
}

function renderCommunicationFrame() {
  communicationLayer.setAttribute('viewBox', `0 0 ${MAP_WIDTH} ${MAP_HEIGHT}`);
  communicationLayer.setAttribute('width', `${MAP_WIDTH}`);
  communicationLayer.setAttribute('height', `${MAP_HEIGHT}`);
  communicationLayer.innerHTML = '';

  const world = playback.world;
  const frames = playback.events;
  const links = playback.communicationLinks;

  if (!world || links.length === 0) {
    return;
  }

  let currentTime = Number.POSITIVE_INFINITY;
  let prevTime = Number.NEGATIVE_INFINITY;

  if (frames.length > 0) {
    const clampedIndex = Math.max(0, Math.min(playback.frameIndex, frames.length - 1));
    currentTime = frames[clampedIndex].time;
    prevTime = clampedIndex > 0 ? frames[clampedIndex - 1].time : Number.NEGATIVE_INFINITY;
  }

  for (const link of links) {
    if (!(link.time > prevTime && link.time <= currentTime)) {
      continue;
    }

    const src = world.toMapPoint(link.srcX, link.srcY);
    const dst = world.toMapPoint(link.dstX, link.dstY);

    const line = document.createElementNS('http://www.w3.org/2000/svg', 'line');
    line.setAttribute('x1', src.x);
    line.setAttribute('y1', src.y);
    line.setAttribute('x2', dst.x);
    line.setAttribute('y2', dst.y);
    line.setAttribute(
      'class',
      link.linkType === 'R' ? 'comm-link comm-link-r' : 'comm-link comm-link-s',
    );
    communicationLayer.appendChild(line);
  }
}

function applyUavPathPlaybackStyle() {
  if (playback.isPlaying) {
    uavPathLayer.classList.add('playing');
  } else {
    uavPathLayer.classList.remove('playing');
  }
}

function updatePlaybackUi() {
  const total = playback.events.length;
  eventSlider.min = '0';
  eventSlider.max = total > 0 ? String(total - 1) : '0';
  eventSlider.value = total > 0 ? String(playback.frameIndex) : '0';
  playPauseBtn.textContent = playback.isPlaying ? 'Pause' : 'Play';
  applyUavPathPlaybackStyle();
  renderCommunicationFrame();
  renderUavEventFrame();
}

function stopPlayback() {
  playback.isPlaying = false;
  if (playback.timer) {
    clearInterval(playback.timer);
    playback.timer = null;
  }
}

function startPlayback() {
  if (playback.events.length === 0) {
    return;
  }
  stopPlayback();
  playback.isPlaying = true;
  playback.timer = setInterval(() => {
    if (playback.frameIndex >= playback.events.length - 1) {
      stopPlayback();
      updatePlaybackUi();
      return;
    }
    playback.frameIndex += 1;
    updatePlaybackUi();
  }, playback.frameIntervalMs);
}

function bindTimelineControls() {
  playPauseBtn.addEventListener('click', () => {
    if (playback.isPlaying) {
      stopPlayback();
      updatePlaybackUi();
      return;
    }
    startPlayback();
    updatePlaybackUi();
  });

  prevFrameBtn.addEventListener('click', () => {
    stopPlayback();
    if (playback.events.length > 0) {
      playback.frameIndex = Math.max(0, playback.frameIndex - 1);
    }
    updatePlaybackUi();
  });

  nextFrameBtn.addEventListener('click', () => {
    stopPlayback();
    if (playback.events.length > 0) {
      playback.frameIndex = Math.min(playback.events.length - 1, playback.frameIndex + 1);
    }
    updatePlaybackUi();
  });

  eventSlider.addEventListener('input', () => {
    stopPlayback();
    playback.frameIndex = Number(eventSlider.value) || 0;
    updatePlaybackUi();
  });
}

async function loadAndRenderNodes() {
  try {
    const response = await fetch('results/scenario4_result_42_1.txt');
    if (!response.ok) {
      throw new Error(`Không đọc được file kết quả (${response.status})`);
    }

    const text = await response.text();
    applyScenarioText(text);
  } catch (err) {
    nodeStats.textContent = `Chọn file .txt để hiển thị`;
  }
}

function applyScenarioText(text) {
  const config = parseConfig(text);
  const nodes = parseNodeInfo(text);
  const leaders = parseCellLeaders(text);
  const suspiciousInfo = parseSuspiciousInfo(text);
  const uavPaths = parseUavPaths(text);
  const waypointEvents = parseUavWaypointEvents(text);
  const communicationLinks = parseCommunicationLinks(text);
  const cells = buildCellInfo(nodes, config.cellRadius);
  const world = buildWorldMapper(nodes);

  renderHexLayer(world, config.cellRadius, cells);
  renderSuspiciousCellLayer(world,
                            config.cellRadius,
                            cells,
                            suspiciousInfo.suspiciousCells);
  renderNodeLayer(nodes, world);
  renderNodeColorLayer(nodes, world, leaders);
  renderSuspiciousNodeLayer(nodes,
                            world,
                            suspiciousInfo.suspiciousNodes,
                            suspiciousInfo.suspiciousPoint);
  renderUavPathLayer(world, uavPaths);

  stopPlayback();
  playback.world = world;
  playback.waypointEvents = waypointEvents;
  playback.communicationLinks = resolveCommunicationGeometry(communicationLinks, nodes, waypointEvents);
  playback.events = buildPlaybackFrames(waypointEvents, playback.communicationLinks);
  playback.frameIndex = 0;
  playback.uavColorMap = buildUavColorMap(uavPaths, waypointEvents);
  updatePlaybackUi();

  nodeStats.textContent = `Nodes: ${nodes.length} | Cells: ${cells.length} | Suspicious Nodes: ${suspiciousInfo.suspiciousNodes.size} | Suspicious Cells: ${suspiciousInfo.suspiciousCells.size} | UAVs: ${uavPaths.length} | Cell Radius: ${config.cellRadius}m`;
}

function bindFileInput() {
  resultFileInput.addEventListener('change', async (event) => {
    const file = event.target.files && event.target.files[0];
    if (!file) {
      return;
    }

    try {
      const text = await file.text();
      applyScenarioText(text);
    } catch (error) {
      nodeStats.textContent = 'Không đọc được file đã chọn';
      console.error(error);
    }
  });
}

function bindLayerToggles() {
  const applyLayerVisibility = () => {
    gridLayer.style.display = toggleGrid.checked ? 'block' : 'none';
    hexLayer.style.display = toggleHex.checked ? 'block' : 'none';
    hexColorLayer.style.display = toggleHexColor.checked ? 'block' : 'none';
    suspiciousCellLayer.style.display = toggleSuspiciousNode.checked ? 'block' : 'none';
    nodeLayer.style.display = toggleNode.checked ? 'block' : 'none';
    nodeColorLayer.style.display = toggleNodeColor.checked ? 'block' : 'none';
    suspiciousNodeLayer.style.display = toggleSuspiciousNode.checked ? 'block' : 'none';
    uavPathLayer.style.display = toggleUavPath.checked ? 'block' : 'none';
    communicationLayer.style.display = toggleCommunication.checked ? 'block' : 'none';
  };

  toggleGrid.addEventListener('change', () => {
    applyLayerVisibility();
  });

  toggleHex.addEventListener('change', () => {
    applyLayerVisibility();
  });

  toggleHexColor.addEventListener('change', () => {
    applyLayerVisibility();
  });

  toggleNode.addEventListener('change', () => {
    applyLayerVisibility();
  });

  toggleNodeColor.addEventListener('change', () => {
    if (toggleNodeColor.checked) {
      toggleSuspiciousNode.checked = false;
    }
    applyLayerVisibility();
  });

  toggleSuspiciousNode.addEventListener('change', () => {
    if (toggleSuspiciousNode.checked) {
      toggleNodeColor.checked = false;
    }
    applyLayerVisibility();
  });

  toggleUavPath.addEventListener('change', () => {
    applyLayerVisibility();
  });

  toggleCommunication.addEventListener('change', () => {
    applyLayerVisibility();
  });

  applyLayerVisibility();
}

viewport.addEventListener('wheel', (event) => {
  event.preventDefault();
  const factor = event.deltaY < 0 ? 1.12 : 0.9;
  zoomAt(event.clientX, event.clientY, factor);
}, { passive: false });

viewport.addEventListener('pointerdown', (event) => {
  state.isDragging = true;
  state.dragStartX = event.clientX - state.x;
  state.dragStartY = event.clientY - state.y;
  viewport.setPointerCapture(event.pointerId);
});

viewport.addEventListener('pointermove', (event) => {
  if (!state.isDragging) {
    return;
  }
  state.x = event.clientX - state.dragStartX;
  state.y = event.clientY - state.dragStartY;
  applyTransform();
});

function stopDragging(event) {
  state.isDragging = false;
  if (event) {
    viewport.releasePointerCapture(event.pointerId);
  }
}

viewport.addEventListener('pointerup', stopDragging);
viewport.addEventListener('pointercancel', stopDragging);
viewport.addEventListener('pointerleave', (event) => {
  if (state.isDragging && event.buttons === 0) {
    stopDragging(event);
  }
});

zoomInBtn.addEventListener('click', () => {
  const rect = viewport.getBoundingClientRect();
  zoomAt(rect.left + rect.width / 2, rect.top + rect.height / 2, 1.15);
});

zoomOutBtn.addEventListener('click', () => {
  const rect = viewport.getBoundingClientRect();
  zoomAt(rect.left + rect.width / 2, rect.top + rect.height / 2, 0.85);
});

resetViewBtn.addEventListener('click', resetView);
window.addEventListener('resize', resetView);

bindLayerToggles();
bindFileInput();
bindTimelineControls();

if (window.location.protocol !== 'file:') {
  loadAndRenderNodes();
} else {
  nodeStats.textContent = 'Mở file result .txt để bắt đầu';
}

resetView();