These fixtures use the configured Verilator's real storage types and helper
definitions. The calls are written explicitly so Verilator optimizations cannot
remove the helper signatures or array bases under test.

`vrtlmod:test/helpers` transforms the fixture, checks its exact XML inventory,
and compiles the modified model and all generated C++ API sources.
`run:test/helpers` checks helper results, then flips one bit of each destination
and requires exactly one injection without changing another target or a source.
The expected destinations are independent of the parser's helper table.
Different array elements are assigned in sequence, so an injection consumed
after an earlier element's write cannot disappear beneath a later assignment.

The negative case writes an internal temporary using a user signal as its array
index. That signal is readable and must never become an injection target.

When the configured headers define the `WDataOutP` wrapper class,
`vrtlmod:test/helpers-three-unpacked` also checks its constructor matcher through
three unpacked array bases. This case uses XML-only analysis: three unpacked
dimensions plus `VlWide` exceed the generated API's three container dimensions.
