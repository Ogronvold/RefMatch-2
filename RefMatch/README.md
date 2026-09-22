# RefMatch 0.5.4

See ../README.md, ../BUILD-NOTES.md and ../INSTALL-DA.txt.

ReferenceAnalysis owns the system-audio analysis thread. MIX processing remains in
PluginProcessor. MatchEQ contains the learned bank followed by three Tone stages.
