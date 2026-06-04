# Kiss radio protocol utilities
Just some help encoding and decoding kiss frames

## Terminology
| Term | Meaning | Comment |
| -----| --------| --------| 
| KISS | Keep It Simple Stupid | *sigh* |
| TNC  | Terminal Node Controller | |

# Useful codes
| Hex value	| Abbreviation	| Description |
| --------- | ------------- | ----------- | 
| 0xC0	| FEND	| Frame End |
| 0xDB	| FESC	| Frame Escape |
| 0xDC	| TFEND	| Transposed Frame End |
| 0xDD	| TFESC	| Transposed Frame Escape |


## References
https://en.wikipedia.org/wiki/KISS_(amateur_radio_protocol)<br/>
https://en.wikipedia.org/wiki/Terminal_node_controller<br/>

## Host unit tests
A simple host test target is available under `tests/`.

Build and run from the repository root:

```bash
cd tests
cmake -S . -B build
cmake --build build && ctest --verbose --test-dir build
```

This compiles `pico-kiss-protocol` as a normal host executable and runs the unit tests without Pico hardware.

## Monitor application
```bash
cmake -S apps/monitor -B apps/monitor/build
cmake --build apps/monitor/build --target monitor
```