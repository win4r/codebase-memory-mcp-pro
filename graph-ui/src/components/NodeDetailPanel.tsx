import { useMemo } from "react";
import { ScrollArea } from "@/components/ui/scroll-area";
import { colorForLabel } from "../lib/colors";
import type { GraphNode, GraphEdge } from "../lib/types";

interface Connection {
  node: GraphNode;
  edgeType: string;
  direction: "inbound" | "outbound";
}

interface NodeDetailPanelProps {
  node: GraphNode;
  allNodes: GraphNode[];
  allEdges: GraphEdge[];
  onClose: () => void;
  onNavigate: (node: GraphNode) => void;
}

export function NodeDetailPanel({ node, allNodes, allEdges, onClose, onNavigate }: NodeDetailPanelProps) {
  const connections = useMemo(() => {
    const nodeMap = new Map<number, GraphNode>();
    for (const n of allNodes) nodeMap.set(n.id, n);
    const conns: Connection[] = [];
    for (const edge of allEdges) {
      if (edge.source === node.id) {
        const t = nodeMap.get(edge.target);
        if (t) conns.push({ node: t, edgeType: edge.type, direction: "outbound" });
      }
      if (edge.target === node.id) {
        const s = nodeMap.get(edge.source);
        if (s) conns.push({ node: s, edgeType: edge.type, direction: "inbound" });
      }
    }
    return conns;
  }, [node, allNodes, allEdges]);

  const outbound = connections.filter((c) => c.direction === "outbound");
  const inbound = connections.filter((c) => c.direction === "inbound");

  const groupByType = (conns: Connection[]) => {
    const g = new Map<string, Connection[]>();
    for (const c of conns) g.set(c.edgeType, [...(g.get(c.edgeType) ?? []), c]);
    return [...g.entries()].sort((a, b) => b[1].length - a[1].length);
  };

  return (
    <div className="w-full bg-[#0b1920]/95 backdrop-blur-xl flex flex-col h-full min-h-0 overflow-hidden">
      {/* Header */}
      <div className="px-4 pt-4 pb-3 border-b border-border/30">
        <div className="flex items-start justify-between gap-2 mb-2">
          <div className="min-w-0 flex-1">
            <div className="flex items-center gap-2 mb-1.5">
              <span className="w-2.5 h-2.5 rounded-full shrink-0" style={{ backgroundColor: colorForLabel(node.label) }} />
              <h3 className="text-[13px] font-semibold text-foreground truncate">{node.name}</h3>
            </div>
            <span
              className="inline-block px-2 py-0.5 rounded-md text-[10px] font-medium"
              style={{ backgroundColor: colorForLabel(node.label) + "18", color: colorForLabel(node.label) }}
            >
              {node.label}
            </span>
          </div>
          <button onClick={onClose} className="text-foreground/20 hover:text-foreground/50 transition-colors text-[16px] leading-none p-1">×</button>
        </div>

        {node.file_path && (
          <p className="text-[11px] text-foreground/30 font-mono mt-2 break-all leading-relaxed">{node.file_path}</p>
        )}

        {/* Stats */}
        <div className="flex gap-5 mt-3">
          {[
            { label: "Out", value: outbound.length, color: "text-primary" },
            { label: "In", value: inbound.length, color: "text-accent" },
            { label: "Total", value: connections.length, color: "text-foreground" },
          ].map((s) => (
            <div key={s.label}>
              <p className="text-[9px] text-foreground/25 uppercase tracking-widest">{s.label}</p>
              <p className={`text-[18px] font-semibold tabular-nums ${s.color}`}>{s.value}</p>
            </div>
          ))}
        </div>
      </div>

      {/* Connections */}
      <ScrollArea className="flex-1 min-h-0">
        <div className="px-4 py-3 space-y-4">
          {outbound.length > 0 && (
            <ConnectionSection title="References" count={outbound.length} icon="→" groups={groupByType(outbound)} onNavigate={onNavigate} />
          )}
          {inbound.length > 0 && (
            <ConnectionSection title="Referenced by" count={inbound.length} icon="←" groups={groupByType(inbound)} onNavigate={onNavigate} />
          )}
          {connections.length === 0 && (
            <p className="text-[12px] text-foreground/20 text-center py-8">No connections</p>
          )}
        </div>
      </ScrollArea>
    </div>
  );
}

function ConnectionSection({ title, count, icon, groups, onNavigate }: {
  title: string; count: number; icon: string;
  groups: [string, Connection[]][];
  onNavigate: (n: GraphNode) => void;
}) {
  return (
    <div>
      <p className="text-[11px] font-medium text-foreground/40 mb-2">
        {title} <span className="text-foreground/15">({count})</span>
      </p>
      {groups.map(([type, conns]) => (
        <div key={type} className="mb-2">
          <p className="text-[9px] text-foreground/20 uppercase tracking-wider mb-1 font-medium">
            {type.replace(/_/g, " ").toLowerCase()}
          </p>
          <div className="space-y-px">
            {conns.slice(0, 25).map((c, i) => (
              <button
                key={`${c.node.id}-${i}`}
                onClick={() => onNavigate(c.node)}
                className="flex items-center gap-1.5 w-full text-left px-2 py-[4px] rounded-md hover:bg-white/[0.04] text-[11px] transition-colors group"
              >
                <span className="text-foreground/15 text-[10px] group-hover:text-foreground/30">{icon}</span>
                <span className="w-[5px] h-[5px] rounded-full shrink-0" style={{ backgroundColor: colorForLabel(c.node.label) }} />
                <span className="text-foreground/55 group-hover:text-foreground/80 truncate">{c.node.name}</span>
                <span className="text-foreground/10 ml-auto text-[10px] shrink-0">{c.node.label}</span>
              </button>
            ))}
            {conns.length > 25 && (
              <p className="text-[10px] text-foreground/15 px-2 py-1">+{conns.length - 25} more</p>
            )}
          </div>
        </div>
      ))}
    </div>
  );
}
