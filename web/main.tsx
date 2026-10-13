import { createRoot } from "react-dom/client"
import { LoginArt } from "@/components/login-art"
import "@/styles/globals.css"
import "./styles.css"
import "@/styles/theme.css"
import "./app.js"

for (const id of ["ascii-art-root", "hero-ascii-art-root"]) {
  const art = document.getElementById(id)
  if (art) createRoot(art).render(<LoginArt />)
}
