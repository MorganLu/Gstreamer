gst-launch-1.0 filesrc location=$1 ! matroskademux name=d                   \
    d.audio_0 ! queue ! vorbisdec ! audioconvert ! queue ! autoaudiosink    \
    d.video_0 ! queue ! vp8dec    ! videoconvert ! queue ! autovideosink    \
