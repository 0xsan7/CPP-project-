import { useEffect, useRef } from "react"
import { AsciiArt } from "@/components/ui/creation-of-adam"

export function LoginArt() {
  const container = useRef<HTMLDivElement>(null)

  useEffect(() => {
    const video = container.current?.querySelector("video")
    const preference = window.matchMedia("(prefers-reduced-motion: reduce)")
    let visible = true
    const sync = () => {
      if (!video) return
      if (preference.matches || document.hidden || !visible) video.pause()
      else void video.play().catch(() => {})
    }
    sync()
    const observer = new IntersectionObserver(([entry]) => {
      visible = entry.isIntersecting
      sync()
    })
    if (container.current) observer.observe(container.current)
    preference.addEventListener("change", sync)
    document.addEventListener("visibilitychange", sync)
    return () => {
      preference.removeEventListener("change", sync)
      document.removeEventListener("visibilitychange", sync)
      observer.disconnect()
    }
  }, [])

  return (
    <div ref={container} className="relative h-full w-full overflow-hidden" aria-hidden="true">
      <AsciiArt className="absolute inset-0 h-full w-full grayscale" />
      <div className="ascii-art-shade absolute inset-0" />
    </div>
  )
}
