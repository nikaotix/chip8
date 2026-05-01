Simple Chip-8 (no Super/XO support) simulator.

Current progress:
- All internal instructions are implemented, and should? in theory, support the weird different implementations.
- Build system is implemented.
- Input is implemented.
- Timers are implemented
- Display works!
- Audio works!

TODO:
- adopt C23 constexpr instead of #define.
- Support synchronized sprite drawing (i.e. slow sprites) for COSMAC modes.
- Improve quirks support (runtime configuration, sane defaults to mimic Chip-8 on SuperChip platforms.)
- Fix carry flag problems on SHL/SHR instructions. Issues are Flag is wrong and vF can't be used as Vy input? in happy path (no carry) for 8xy6, vF can't be used as Vy input for 8xyE happy path?, and flag is wrong on carry for 8xyE. Behavior changes if the COSMAC behavior of moving the value of vY into vX then shifting happens. This may? be an issue with the testing, though.

