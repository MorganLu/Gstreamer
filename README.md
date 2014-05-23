Gstreamer
=========

Gstreamer version: 1.2.3 (core, base, and good plugins)

Simple pipeline with matroska plugin(demux + mux)

In main.c file, I implement pipeline like its comment with matroskademux and matroskamux.

Define: ACTIVE_AUDIO, ACTIVE_VIDEO, ACTIVE_BOTH could help programmer change code instantly

main.c have an problem:
  It works fine with only audio or video, but when I try to mux audio and video to the same webm file(ACTIVE_BOTH=1), pipeline stucks.
  
  I couldn't understand why it happend.
