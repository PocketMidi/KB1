---
applyTo:
  - "KB1-config/src/styles/**/*.css"
  - "KB1-config/src/assets/ui/**"
  - "KB1-config/DESIGNER_WORKFLOW.md"
  - "KB1-config/THEME_IMPLEMENTATION.md"
---

# KB1 Theme & CSS Expert

Expert knowledge for KB1's CSS-only theme system designed for designers to update visuals without touching code.

## Theme Philosophy

**CSS-ONLY** — Zero JavaScript, maximum designer autonomy.

**Goal:** Designers export from Figma → update CSS variables → done!

**Benefits:**
- No component code changes required
- Single source of truth (`kb1.css`)
- Easy to maintain and update
- Visual updates without rebuilding app logic

## File Structure

```
KB1-config/
├── src/
│   ├── styles/
│   │   └── themes/
│   │       └── kb1.css          ← DESIGNERS EDIT THIS FILE
│   ├── assets/
│   │   └── ui/                  ← DESIGNERS PUT IMAGES HERE
│   │       ├── lever-panel.svg
│   │       ├── lever-push-panel.svg
│   │       ├── touch-panel.svg
│   │       ├── scale-panel.svg
│   │       └── ...
│   └── components/
│       └── *.vue                ← Components USE theme variables
└── App.vue                      ← Imports kb1.css
```

## Theme Application

### App.vue (Root Component)

```vue
<template>
  <div id="app" class="theme-kb1">
    <!-- All components inherit theme -->
    <router-view />
  </div>
</template>

<style>
@import './styles/themes/kb1.css';
</style>
```

### Component Usage

Components reference theme variables **without hardcoding**:

```vue
<style scoped>
.settings-panel {
  background: var(--lever-panel-bg, #242424);
  padding: var(--kb1-spacing-lg, 1.5rem);
  border-radius: var(--kb1-border-radius, 8px);
  color: var(--kb1-text-primary, #ffffff);
}

.primary-button {
  background: var(--btn-primary-bg);
  color: var(--btn-primary-color);
}

.primary-button:hover {
  background: var(--btn-primary-bg-hover);
}
</style>
```

**Key point:** Fallback values ensure graceful degradation.

## Available Theme Variables

### Panel Backgrounds

```css
/* Panel backgrounds for settings sections */
--lever-panel-bg: url('/src/assets/ui/lever-panel.svg');
--lever-push-panel-bg: url('/src/assets/ui/lever-push-panel.svg');
--touch-panel-bg: url('/src/assets/ui/touch-panel.svg');
--scale-panel-bg: url('/src/assets/ui/scale-panel.svg');
--keyboard-panel-bg: url('/src/assets/ui/keyboard-panel.svg');
--system-panel-bg: url('/src/assets/ui/system-panel.svg');
```

**Usage:** Background images behind each settings section.

### Fader Elements

```css
/* Custom fader/slider controls */
--fader-track-bg: url('/src/assets/ui/fader-track.svg');
--fader-thumb-bg: url('/src/assets/ui/fader-thumb.svg');
--fader-track-height: 200px;
--fader-thumb-size: 24px;
```

### Color Tokens

```css
/* Primary brand colors */
--kb1-color-primary: #3b82f6;      /* Blue - primary actions */
--kb1-color-accent: #8b5cf6;       /* Purple - highlights */
--kb1-color-success: #10b981;      /* Green - success states */
--kb1-color-warning: #f59e0b;      /* Orange - warnings */
--kb1-color-error: #ef4444;        /* Red - errors */

/* Text colors */
--kb1-text-primary: #ffffff;       /* Main text */
--kb1-text-secondary: #a1a1aa;     /* Secondary text */
--kb1-text-muted: #71717a;         /* Muted/disabled text */

/* Background colors */
--kb1-bg-primary: #1a1a1a;         /* Main background */
--kb1-bg-secondary: #242424;       /* Card/panel background */
--kb1-bg-tertiary: #2e2e2e;        /* Hover states */

/* Border colors */
--kb1-border-color: #3f3f46;       /* Default borders */
--kb1-border-focus: #3b82f6;       /* Focused element borders */
```

### Button Styles

```css
/* Primary buttons (Apply, Save to Flash) */
--btn-primary-bg: linear-gradient(135deg, #3b82f6 0%, #2563eb 100%);
--btn-primary-bg-hover: linear-gradient(135deg, #2563eb 0%, #1d4ed8 100%);
--btn-primary-color: #ffffff;
--btn-primary-shadow: 0 4px 6px rgba(59, 130, 246, 0.3);

/* Secondary buttons (Load, Reset) */
--btn-secondary-bg: var(--kb1-bg-secondary);
--btn-secondary-bg-hover: var(--kb1-bg-tertiary);
--btn-secondary-border: 1px solid var(--kb1-border-color);
--btn-secondary-color: var(--kb1-text-primary);

/* Danger buttons (Delete) */
--btn-danger-bg: #ef4444;
--btn-danger-bg-hover: #dc2626;
--btn-danger-color: #ffffff;
```

### Typography

```css
/* Font sizing scale */
--kb1-font-size-xs: 0.75rem;    /* 12px - small labels */
--kb1-font-size-sm: 0.875rem;   /* 14px - body text */
--kb1-font-size-base: 1rem;     /* 16px - default */
--kb1-font-size-lg: 1.125rem;   /* 18px - headings */
--kb1-font-size-xl: 1.25rem;    /* 20px - section titles */
--kb1-font-size-2xl: 1.5rem;    /* 24px - page titles */

/* Font weights */
--kb1-font-weight-normal: 400;
--kb1-font-weight-medium: 500;
--kb1-font-weight-bold: 700;

/* Line heights */
--kb1-line-height-tight: 1.25;
--kb1-line-height-normal: 1.5;
--kb1-line-height-relaxed: 1.75;
```

### Spacing

```css
/* Consistent spacing scale */
--kb1-spacing-xs: 0.25rem;   /* 4px */
--kb1-spacing-sm: 0.5rem;    /* 8px */
--kb1-spacing-md: 1rem;      /* 16px */
--kb1-spacing-lg: 1.5rem;    /* 24px */
--kb1-spacing-xl: 2rem;      /* 32px */
--kb1-spacing-2xl: 3rem;     /* 48px */
```

### Layout

```css
/* Border radius */
--kb1-border-radius: 8px;
--kb1-border-radius-sm: 4px;
--kb1-border-radius-lg: 12px;

/* Shadows */
--kb1-shadow-sm: 0 1px 2px rgba(0, 0, 0, 0.1);
--kb1-shadow-md: 0 4px 6px rgba(0, 0, 0, 0.1);
--kb1-shadow-lg: 0 10px 15px rgba(0, 0, 0, 0.2);

/* Transitions */
--kb1-transition-fast: 150ms ease-in-out;
--kb1-transition-normal: 300ms ease-in-out;
--kb1-transition-slow: 500ms ease-in-out;
```

### Z-Index Layers

```css
/* Layering hierarchy */
--kb1-z-base: 1;
--kb1-z-dropdown: 10;
--kb1-z-sticky: 100;
--kb1-z-modal: 1000;
--kb1-z-toast: 2000;
--kb1-z-tooltip: 3000;
```

## Designer Workflow

### Step 1: Design in Figma

Create your visual assets in Figma:
- Panel backgrounds (SVG preferred)
- Button states
- Fader components
- Icons and graphics

### Step 2: Export Assets

Export from Figma to `src/assets/ui/`:

```
src/assets/ui/
├── lever-panel.svg          ← Lever 1/2 background
├── lever-push-panel.svg     ← Push 1/2 background
├── touch-panel.svg          ← Touch sensor background
├── scale-panel.svg          ← Scale settings background
├── fader-track.svg          ← Fader track graphic
├── fader-thumb.svg          ← Fader thumb/handle
└── button-primary.svg       ← Optional: button graphics
```

**File format recommendations:**
- **SVG** (best): Scalable, small file size, crisp at any resolution
- **PNG**: Good for photos or complex gradients
- **WebP**: Modern format with excellent compression

### Step 3: Update Theme CSS

Edit `src/styles/themes/kb1.css`:

```css
/* Update image paths */
--lever-panel-bg: url('/src/assets/ui/lever-panel-new.svg');

/* Or update colors */
--kb1-color-primary: #ff6b6b;  /* New brand color */

/* Or update spacing */
--kb1-spacing-md: 1.5rem;  /* More breathing room */
```

### Step 4: Preview Changes

```bash
cd KB1-config
npm run dev
```

Open http://localhost:5173/KB1-config/ and see changes live!

**Hot reload:** Changes appear automatically (no page refresh needed).

## Naming Conventions

### File Names

Use descriptive, kebab-case names:

**✅ Good:**
- `lever-panel-dark-mode.svg`
- `button-primary-hover.svg`
- `fader-track-gradient.svg`

**❌ Bad:**
- `Panel1.svg`
- `image_final_v3.svg`
- `New Design.svg`

### CSS Variable Names

Follow BEM-inspired convention:

```
--{component}-{property}-{modifier}
```

**Examples:**
- `--btn-primary-bg`
- `--lever-panel-bg`
- `--kb1-text-secondary`

## Asset Organization

### Organized Approach (Recommended)

```
src/assets/ui/
├── panels/
│   ├── lever.svg
│   ├── lever-push.svg
│   ├── touch.svg
│   └── scale.svg
├── buttons/
│   ├── primary.svg
│   ├── primary-hover.svg
│   ├── secondary.svg
│   └── secondary-hover.svg
├── faders/
│   ├── track.svg
│   └── thumb.svg
└── icons/
    ├── bluetooth.svg
    ├── settings.svg
    └── preset.svg
```

**Update CSS paths:**
```css
--lever-panel-bg: url('/src/assets/ui/panels/lever.svg');
--btn-primary-bg: url('/src/assets/ui/buttons/primary.svg');
--fader-track-bg: url('/src/assets/ui/faders/track.svg');
```

## Colors vs Images

### When to use CSS color variables:
- ✅ Solid colors (faster, easier to adjust)
- ✅ Simple gradients (2-3 color stops)
- ✅ Theme switching (easy to override)

### When to use image assets:
- ✅ Complex gradients or textures
- ✅ Illustrations or graphics
- ✅ Brand elements requiring exact visual match

**Example mixing both:**
```css
.panel {
  /* Solid color background */
  background-color: var(--kb1-bg-secondary);
  
  /* With optional image overlay */
  background-image: var(--lever-panel-bg);
  background-size: cover;
  background-position: center;
}
```

## Responsive Design

### Breakpoints

```css
/* Mobile */
@media (max-width: 640px) {
  :root {
    --kb1-spacing-lg: 1rem;     /* Reduce spacing */
    --kb1-font-size-xl: 1rem;   /* Smaller headings */
  }
}

/* Tablet */
@media (max-width: 768px) {
  :root {
    --kb1-spacing-xl: 1.5rem;
  }
}

/* Desktop */
@media (min-width: 1024px) {
  :root {
    --kb1-spacing-2xl: 4rem;    /* More generous spacing */
  }
}
```

### Scalable Assets

For SVG assets, ensure they scale properly:

```css
.panel-background {
  background-image: var(--lever-panel-bg);
  background-size: contain;      /* Scale to fit */
  background-repeat: no-repeat;  /* Don't tile */
  background-position: center;   /* Center the image */
}
```

## Testing Your Changes

### Visual Testing Checklist

- [ ] Panel backgrounds appear correctly
- [ ] Button states (normal, hover, active, disabled)
- [ ] Colors have sufficient contrast (WCAG AA: 4.5:1 for text)
- [ ] Spacing feels consistent
- [ ] Typography is readable
- [ ] Faders/sliders work smoothly
- [ ] Test at different window sizes (responsive)
- [ ] Check on different displays (Retina, standard)
- [ ] Test in both light and dark modes (if applicable)

### Browser DevTools

1. Open Chrome DevTools (F12)
2. Navigate to "Elements" tab
3. Find `:root` styles to see computed CSS variables
4. Edit variable values live to experiment
5. Copy working values back to `kb1.css`

### Component Inspector

```vue
<template>
  <div class="debug-panel">
    <h3>Theme Debug</h3>
    <div :style="{ 
      background: 'var(--lever-panel-bg)', 
      height: '200px' 
    }">
      Background Preview
    </div>
  </div>
</template>
```

## Troubleshooting

### Image doesn't appear

**Check:**
- ✅ File path in CSS matches actual file location
- ✅ File extension is correct (`.svg` vs `.SVG`)
- ✅ File isn't corrupted (open in browser directly)
- ✅ Path starts with `/src/assets/ui/` (absolute from project root)

**Debug:**
```css
/* Add background color to see if element exists */
.panel {
  background: var(--lever-panel-bg);
  background-color: red;  /* If you see red, image path is wrong */
}
```

### Colors look wrong

**Check:**
- ✅ Editing the right variable (some elements inherit from base vars)
- ✅ No inline styles overriding CSS variables
- ✅ Component scoped styles aren't overriding theme
- ✅ Color format is valid (hex, rgb, hsl)

**Debug:**
```javascript
// In browser console
getComputedStyle(document.documentElement).getPropertyValue('--kb1-color-primary')
```

### Spacing inconsistent

**Check:**
- ✅ Using theme variables (`var(--kb1-spacing-md)`)
- ✅ Not mixing `px`, `rem`, and `em` units
- ✅ Component styles use theme spacing tokens

**Fix:**
```css
/* ❌ Bad - hardcoded values */
.panel {
  padding: 24px;
  margin: 16px;
}

/* ✅ Good - theme variables */
.panel {
  padding: var(--kb1-spacing-lg);
  margin: var(--kb1-spacing-md);
}
```

### Hot reload not working

**Try:**
1. Save the CSS file again (Cmd/Ctrl + S)
2. Hard refresh browser (Cmd/Ctrl + Shift + R)
3. Restart dev server (`npm run dev`)
4. Clear browser cache

## Performance Considerations

### Image Optimization

**SVG:**
- Run through SVGO: `npx svgo input.svg -o output.svg`
- Remove unnecessary metadata
- Compress paths

**PNG:**
- Use ImageOptim or TinyPNG
- Use appropriate dimensions (don't use 4K images for small icons)

**WebP:**
- Convert from PNG: `cwebp input.png -o output.webp -q 80`

### CSS Variables Performance

CSS variables are very fast, but avoid:
- ❌ Excessive nesting (more than 3-4 levels)
- ❌ Calc() with multiple variables in loops
- ❌ Changing variables on every frame (animations)

**Prefer:**
- ✅ Static variable definitions
- ✅ CSS transitions over JavaScript animations
- ✅ Transform/opacity for animations (GPU accelerated)

## Best Practices

1. **Single source of truth:** All theme values in `kb1.css`
2. **Semantic naming:** Use purpose-based names, not appearance-based
   - ✅ `--btn-primary-bg` (purpose)
   - ❌ `--blue-500` (appearance)
3. **Fallback values:** Always provide fallbacks in components
4. **Organize assets:** Use folders to keep assets organized
5. **Document changes:** Comment CSS for non-obvious decisions
6. **Test responsively:** Check all breakpoints
7. **Optimize assets:** Compress images before committing
8. **Version assets:** Include version in filename for cache busting

---

**Remember:** Components should NEVER hardcode visual values. Always use theme variables to keep the design system flexible and maintainable!
