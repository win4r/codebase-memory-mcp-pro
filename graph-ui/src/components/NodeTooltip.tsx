import { Html } from "@react-three/drei";
import type { GraphNode } from "../lib/types";
import { colorForLabel } from "../lib/colors";

interface NodeTooltipProps {
  node: GraphNode;
}

export function NodeTooltip({ node }: NodeTooltipProps) {
  return (
    <Html
      position={[node.x, node.y + node.size * 0.7, node.z]}
      center
      style={{ pointerEvents: "none" }}
    >
      <div className="bg-[#1a1a2e]/95 backdrop-blur border border-white/10 rounded-lg px-3 py-2 text-xs whitespace-nowrap shadow-xl max-w-[350px]">
        <div className="flex items-center gap-1.5 mb-1">
          <span
            className="w-2 h-2 rounded-full shrink-0"
            style={{ backgroundColor: colorForLabel(node.label) }}
          />
          <span className="text-white font-medium truncate">{node.name}</span>
          <span className="text-white/30 ml-1 shrink-0">{node.label}</span>
        </div>
        {node.file_path && (
          <p className="text-white/30 font-mono truncate">{node.file_path}</p>
        )}
      </div>
    </Html>
  );
}
