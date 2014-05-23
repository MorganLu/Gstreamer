Gstreamer
=========

Gstreamer version: 1.2.3 (core, base, and good plugins)

Simple pipeline with matroska plugin(demux + mux)

In main.c file, I implement pipeline like its comment with matroskademux and matroskamux.

Define: ACTIVE_AUDIO, ACTIVE_VIDEO, ACTIVE_BOTH could help programmer change code instantly

main.c have an problem:
  It works fine with only audio or video, but when I try to mux audio and video to the same webm file(ACTIVE_BOTH=1), pipeline stucks.
  
  I couldn't understand why it happend.

Pipeline Graph
=========
You could find graph of pipeline in main.c at pipeline.png inside repository

How to Run main.c
=========
1. You need to prepare an webm file, and rename it to "input.webm" with same folder to main.c
2. compile main.c with command: g++ -g -Wall main.c -o main $(pkg-config --cflags --libs gstreamer-1.0 gstreamer-base-1.0)
3. You will see the execuable file "main" at the current folder after step 2.
4. run the file with command: ./main
5. If success, you could see some output messages as follows:

Create success....
Setup Bins success....
Link elements success....
Start Playing success....
	 cb_new_pad - Create pad video_0 of plug-demux
	 cb_new_pad - Try to link srcpad to element plug-queue1: success
	 cb_new_pad - Create pad audio_0 of plug-demux
	 cb_new_pad - Try to link srcpad to element plug-queue0: success
	 cb_new_pad - dstPad already linked, ignore....

6. Now you could use Ctrl+c to trigger output-selector to switch pad from fakesink to filesink.

^Ctrigger_start_cb start
set pipeline - paused
set mux      - ready
set filesink - null
set mux      - play
set filesink - play
set pipeline - play
trigger_start_cb ends
// stream now write to file result_x.webm(x: 0->n-1) and data size will increase with time
// or stuck when ACTIVE_BOTH=1

7. If pipeline line works fine, you could see messages below:

Setup message bus success....
End-Of-Stream reached.
Try to end pipeline

8. If you want to exit program before it ends, you can press Ctrl+\

Shell Script Usage
=========
1. play_audio.sh: coulde play webm only contains audio: sh play_audio.sh audio_file_name
2. play_video.sh: coulde play webm only contains video: sh play_video.sh video_file_name
3. play_both.sh:  coulde play webm contains audio and video: sh play_both.sh webm_file_name

Reference Links
=========
1. Webm media file from gstreamer sdk: http://docs.gstreamer.com/media/sintel_trailer-480p.webm
