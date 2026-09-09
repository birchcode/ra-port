# Widescreen title

`title-wide-854x400.idx` contains 341,600 row-major palette indices without a header. It uses the original TITLE.PCX palette loaded at runtime. Original game data is still required.

Generated 2026-09-09 with imagegen using the original title as reference, then selected by the user. Converted to 854×400 with Lanczos, quantized to the original palette with Floyd–Steinberg dithering. Original Westwood plaque (14,15)-(160,52) and copyright (220,382)-(424,392) were copied without resampling into (18,18) and (325,370), respectively. Those rectangles and the palette were verified byte-identical to the source. The generated copyright was cleared before replacement. The main title wordmark remains generated artwork. No CRT effect is baked in.

At 5:6 pixel aspect the composition is approximately 16:9. Other wide viewports resize the composition. Main-menu code validates exact file length and uses the original title if absent/invalid. `RA_TITLE_ART=original` bypasses it. Keep this directory alongside the resource root when packaging.
