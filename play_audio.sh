gst-launch-1.0 filesrc location=$1 ! matroskademux name=d d.audio_0 ! queue ! vorbisdec ! audioconvert ! queue ! alsasink
