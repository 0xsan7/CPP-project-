"use client"

// AsciiArt — "creation of adam", made with the 21st ASCII editor and baked
// to its exact rendered output (looping video + poster). Zero dependencies:
// one <video> that fills its parent. Drop it behind or inside your content:
// <div className="relative h-96"><AsciiArt className="absolute inset-0" /></div>
// Remix the source recipe (styles, animation, palette) in the editor:
// https://21st.dev/community/ascii/editor?from=518efad9-3960-4737-937c-3c4a47e84ade
export function AsciiArt({ className }: { className?: string }) {
  return (
    <video
      className={className}
      src={"https://assets.21st.dev/ascii-recipes/videos/user_3KXas6aun6ZlgaGCDSEKRz2himn/cefa5510-e885-4d7f-b083-e7de2c8a3ad2.mp4"}
      poster={"https://assets.21st.dev/ascii-recipes/thumbnails/user_3KXas6aun6ZlgaGCDSEKRz2himn/5942803a-25c0-48e7-8bed-b02cfd6de24b.webp"}
      autoPlay
      loop
      muted
      playsInline
      aria-label={"creation of adam — animated ASCII art"}
      style={{
        display: "block",
        width: "100%",
        height: "100%",
        objectFit: "cover",
      }}
    />
  )
}
