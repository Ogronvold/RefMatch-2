# RefMatch 0.5.17

See ../README.md, ../BUILD-NOTES.md and ../INSTALL-DA.txt.

ReferenceAnalysis owns the system-audio analysis thread. MIX processing remains in
PluginProcessor. MatchEQ contains the learned bank followed by three Tone stages.
