/* Node label → color mapping for sidebar/tooltips (structural meaning) */

const LABEL_COLORS: Record<string, string> = {
  Project: "#e11d48",
  Package: "#f97316",
  Module: "#f97316",
  Folder: "#22c55e",
  File: "#3b82f6",
  Class: "#a855f7",
  Interface: "#a855f7",
  Function: "#06b6d4",
  Method: "#06b6d4",
  Route: "#eab308",
  Variable: "#64748b",
};

const DEFAULT_COLOR = "#94a3b8";

export function colorForLabel(label: string): string {
  return LABEL_COLORS[label] ?? DEFAULT_COLOR;
}

/* Stellar spectral type legend (for the graph view) */
export const STELLAR_LEGEND = [
  { type: "O (Blue Giant)", color: "#80a0ff", description: "50+ connections" },
  { type: "B (Blue-White)", color: "#c0d0ff", description: "26-50 connections" },
  { type: "A (White)", color: "#e8e8ff", description: "13-25 connections" },
  { type: "F (Yellow-White)", color: "#fff0c0", description: "7-12 connections" },
  { type: "G (Yellow/Sun)", color: "#ffe080", description: "4-6 connections" },
  { type: "K (Orange)", color: "#ffa060", description: "2-3 connections" },
  { type: "M (Red Dwarf)", color: "#ff6050", description: "0-1 connections" },
];
