import { useMemo } from "react";
import { Billboard, Text } from "@react-three/drei";
import type { GraphNode } from "../lib/types";

interface NodeLabelsProps {
  nodes: GraphNode[];
  highlightedIds: Set<number> | null;
  maxLabels?: number;
}

export function NodeLabels({
  nodes,
  highlightedIds,
  maxLabels = 80,
}: NodeLabelsProps) {
  const labeled = useMemo(() => {
    const hasHighlight = highlightedIds && highlightedIds.size > 0;

    if (hasHighlight) {
      /* Show labels for all highlighted nodes (up to limit) */
      return nodes
        .filter((n) => highlightedIds.has(n.id))
        .sort((a, b) => b.size - a.size)
        .slice(0, maxLabels);
    }

    /* No selection: show top nodes by size */
    return [...nodes].sort((a, b) => b.size - a.size).slice(0, maxLabels);
  }, [nodes, highlightedIds, maxLabels]);

  return (
    <group>
      {labeled.map((node) => (
        <Billboard
          key={node.id}
          position={[node.x, node.y + node.size * 0.7, node.z]}
          follow
        >
          <Text
            fontSize={Math.max(1.8, node.size * 0.4)}
            color={node.color}
            anchorX="center"
            anchorY="bottom"
            outlineWidth={0.2}
            outlineColor="#000000"
            fillOpacity={0.95}
          >
            {node.name}
          </Text>
        </Billboard>
      ))}
    </group>
  );
}
