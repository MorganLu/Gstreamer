#include <gst/gst.h>
#include <gst/base/gstbasesrc.h>
#include <gst/base/gstbasesink.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>


#define DATA_SRC "./input.webm"

#define ACTIVE_AUDIO 0
#define ACTIVE_VIDEO 0
#define ACTIVE_BOTH  1

static GstElement 
    *pipeline,
    *srcElnt,    	// element - filesrc
    *queElnt[3],	// element - queue
    *demuxElnt,     // element - matroskademux
    *muxElnt,       // element - matroskamux
    *selectorElnt,  // element - output selector
    *fakesinkElnt,  // element - fakesink
    *filesinkElnt;  // element - filesink
        

// srcPad[0]->fakesinkElnt, srcPad[1]->filesinkElnt
static GstPad *srcPad[4], *sinkPad[4];


static void
cb_new_pad( GstElement *srcElnt, GstPad *pad, GstElement *dstElnt ) {
	gchar* pad_name  = gst_pad_get_name(pad);
	gchar* elnt_name = gst_element_get_name(srcElnt);
	GstPad *dstPad = gst_element_get_static_pad( dstElnt, "sink" );

	if ( gst_pad_is_linked(dstPad) ) {
		g_print("\t cb_new_pad - dstPad already linked, ignore....\n");
		goto end;
	}
    else if ( GST_PAD_LINK_SUCCESSFUL(gst_pad_link(pad, dstPad)) ) {
        g_print("\t cb_new_pad - Create pad %s of %s\n", pad_name, elnt_name );
        g_print("\t cb_new_pad - Try to link srcpad to element %s: success\n", gst_element_get_name(dstElnt));
    }

end:
    if (dstPad   != NULL)   gst_object_unref(dstPad);
	g_free(pad_name);
	g_free(elnt_name);

}

static guint64 count_trigger = 0;
static void trigger_start_cb (int sig);
static void trigger_end_cb   (int sig);

static void 
trigger_start_cb(int sig)
{
    g_print("trigger_start_cb start\n");

    // Reset muxElnt
    // switch active path of output-selector: fakesinkElnt -> filesinkElnt
    g_print("set pipeline - paused\n");
    gst_element_set_state(GST_ELEMENT(pipeline), GST_STATE_PAUSED);
    g_print("set mux      - ready\n");
    gst_element_set_state(GST_ELEMENT(muxElnt), GST_STATE_READY);
    g_object_set(selectorElnt   , "active-pad", srcPad[1], NULL);

    // Reset filesink with new file name
    char buf[1024] = {0};
    sprintf(buf, "./result_%llu.webm", count_trigger++);
    g_print("set filesink - null\n");
    gst_element_set_state(filesinkElnt, GST_STATE_NULL);
    g_object_set(GST_ELEMENT(filesinkElnt), "location", buf, "async", FALSE, NULL);

    g_print("set mux      - play\n");
    gst_element_set_state(GST_ELEMENT(muxElnt), GST_STATE_PLAYING);
    g_print("set filesink - play\n");    
    gst_element_set_state(GST_ELEMENT(filesinkElnt), GST_STATE_PLAYING);
    g_print("set pipeline - play\n");
    gst_element_set_state(GST_ELEMENT(pipeline), GST_STATE_PLAYING);

    // Bind SIGINT to trigger_start_cb
    if ( signal(SIGINT, trigger_end_cb) == SIG_ERR ) {
        g_print("bindng signal:SIGSTOP to trigger_start_cb failed\n");
    }

    g_print("trigger_start_cb ends\n\n");
}

static void
trigger_end_cb (int sig)
{
    g_print("trigger_end_cb start\n");

    // Reset muxElnt
    // switch active path of output-selector: filesinkElnt -> fakesinkElnt
    g_print("set pipeline - paused\n");
    gst_element_set_state(GST_ELEMENT(pipeline), GST_STATE_PAUSED);
    g_print("set mux      - ready\n");
    gst_element_set_state(GST_ELEMENT(muxElnt), GST_STATE_READY);

    g_object_set(selectorElnt   , "active-pad", srcPad[0], NULL);

    g_print("set mux      - play\n");
    gst_element_set_state(GST_ELEMENT(muxElnt), GST_STATE_PLAYING);
    g_print("set pipeline - play\n");
    gst_element_set_state(GST_ELEMENT(pipeline), GST_STATE_PLAYING);

    // Bind SIGINT to trigger_start_cb
    if ( signal(SIGINT, trigger_end_cb) == SIG_ERR ) {
        g_print("bindng signal:SIGSTOP to trigger_start_cb failed\n");
    }

    g_print("trigger_end_cb ends\n");

}

// Link Pads with [requested-src => static-sink]
bool link_pads_request_srcpad(
    GstElement *src_ele, GstPad *&srcpad, const char* req_template, const char* req_pad, 
    GstElement *sink_ele, GstPad *&sinkpad) 
{
    bool bLinkSuccess = true;
    srcpad  = gst_element_get_request_pad(src_ele, req_template);
    sinkpad = gst_element_get_static_pad(sink_ele, "sink");
    if ( GST_PAD_LINK_FAILED(gst_pad_link(srcpad, sinkpad)) ) {
        g_print("try to link to %s:%s with %s:%s failed\n", 
            GST_ELEMENT_NAME(src_ele)   , GST_PAD_NAME(srcpad),
            GST_ELEMENT_NAME(sink_ele)  , GST_PAD_NAME(sinkpad)
        );
        bLinkSuccess = false;
    }
    srcpad = gst_element_get_static_pad(src_ele, req_pad);
    g_object_unref(sinkpad);

    return bLinkSuccess;
}

// Link Pads with [static-src => requested-sink]
bool link_pads_request_sinkpad(
    GstElement *src_ele, GstPad *&srcpad, 
    GstElement *sink_ele, GstPad *&sinkpad, const char* req_template, const char* req_pad)
{
    bool bLinkSuccess = true;
    srcpad  = gst_element_get_static_pad(src_ele, "src");
    sinkpad = gst_element_get_request_pad(sink_ele, req_template);
    if ( GST_PAD_LINK_FAILED(gst_pad_link(srcpad, sinkpad)) ) {
        g_print("try to link to %s:%s with %s:%s failed\n", 
            GST_ELEMENT_NAME(src_ele)   , GST_PAD_NAME(srcpad),
            GST_ELEMENT_NAME(sink_ele)  , GST_PAD_NAME(sinkpad)
        );
        bLinkSuccess = false;
    }
    sinkpad = gst_element_get_static_pad(sink_ele, req_pad);
    g_object_unref(srcpad);

    return bLinkSuccess;
}



int main( int argc, char** argv ) {
    char buf[1024] = {0};
	GstBus *bus;
	GstMessage *msg;

	// Init Gstreamer
	gst_init(&argc, &argv);

	// Create
    pipeline        = gst_pipeline_new("test-pipeline");
    srcElnt         = gst_element_factory_make("filesrc"            , "plug-filesrc"    );
    demuxElnt       = gst_element_factory_make("matroskademux"      , "plug-demux"      );
    muxElnt         = gst_element_factory_make("matroskamux"        , "plug-mux"        );
    selectorElnt    = gst_element_factory_make("output-selector"    , "plug-selector"   );
    fakesinkElnt    = gst_element_factory_make("fakesink"           , "plug-fakesink"   );
    filesinkElnt    = gst_element_factory_make("filesink"           , "plug-filesink"   );
    queElnt[0]      = gst_element_factory_make("queue"              , "plug-queue0"     );
    queElnt[1]      = gst_element_factory_make("queue"              , "plug-queue1"     );
    queElnt[2]      = gst_element_factory_make("queue"              , "plug-queue2"     );
	g_print("Create success....\n");


	// Setup properties of elements
    g_object_set(srcElnt        , "location", DATA_SRC, NULL);
    g_object_set(selectorElnt   , "pad-negotiation-mode", 1, NULL);
    g_object_set(fakesinkElnt   , "async", FALSE, NULL);
    
    // set file name
    sprintf(buf, "./result_%llu.webm", count_trigger);
    g_object_set(GST_ELEMENT(filesinkElnt), "location", buf, "async", FALSE, NULL);

   
    gst_bin_add_many(
        GST_BIN(pipeline),
        GST_ELEMENT(srcElnt),
        GST_ELEMENT(demuxElnt),
        GST_ELEMENT(muxElnt),
        GST_ELEMENT(selectorElnt),
        GST_ELEMENT(fakesinkElnt),
        GST_ELEMENT(filesinkElnt),
        GST_ELEMENT(queElnt[0]),
        GST_ELEMENT(queElnt[1]),
        GST_ELEMENT(queElnt[2]),        
        NULL
    );
	g_print("Setup Bins success....\n");



    gst_element_link_many(srcElnt, demuxElnt, NULL);
    gst_element_link_many(muxElnt, queElnt[2], selectorElnt, NULL);
	g_print("Link elements success....\n");

#if (ACTIVE_AUDIO || ACTIVE_BOTH)
    // Bind signal for sometimes pad - audio
    g_signal_connect(demuxElnt, "pad-added", G_CALLBACK(cb_new_pad), queElnt[0]);
    link_pads_request_sinkpad(queElnt[0], srcPad[2], muxElnt, sinkPad[2], "audio_%u", "audio_0");
#endif

#if (ACTIVE_VIDEO || ACTIVE_BOTH)
    // Bind signal for sometimes pad - video  
    g_signal_connect(demuxElnt, "pad-added", G_CALLBACK(cb_new_pad), queElnt[1]);
    link_pads_request_sinkpad(queElnt[1], srcPad[3], muxElnt, sinkPad[3], "video_%u", "video_0");
#endif

    // Link Pads with [requested-src => static-sink]
    // default pad: fakesink
    link_pads_request_srcpad(selectorElnt, srcPad[0], "src_%u", "src_0", fakesinkElnt, sinkPad[0]);
    link_pads_request_srcpad(selectorElnt, srcPad[1], "src_%u", "src_1", filesinkElnt, sinkPad[1]);
    g_object_set(selectorElnt   , "active-pad", srcPad[0], NULL);
   

    // Start Playing
	gst_element_set_state(pipeline, GST_STATE_PLAYING);
    g_print("Start Playing success....\n");

   
    // Bind SIGINT to trigger_start_cb
    if ( signal(SIGINT, trigger_start_cb) == SIG_ERR ) {
        g_print("bindng signal:SIGSTOP to trigger_start_cb failed\n");
    }
   
	// Setup Msg Bus, and watch on Err Msg. and EOS Msg	
    bus = gst_element_get_bus(pipeline);    
	msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS) );
	g_print("Setup message bus success....\n");

	// Parse messages
	if (msg != NULL) {
		GError *err;
		gchar *debug_info;
		
		switch(GST_MESSAGE_TYPE(msg)) {
			case GST_MESSAGE_ERROR:
				gst_message_parse_error (msg, &err, &debug_info);
				fprintf(stderr, "Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
				fprintf(stderr, "Debugging information: %s\n", debug_info ? debug_info : "none");
				g_clear_error (&err);
				g_free (debug_info);
			break;
			case GST_MESSAGE_EOS:
				fprintf(stderr, "End-Of-Stream reached.\n");
				break;
			default:
				/* We should not reach here because we only asked for ERRORs and EOS */
				fprintf(stderr, "Unexpected message received.\n");
				break;	
		}
	}

    // Release   
	if (msg != NULL)        gst_message_unref(msg);
	if (bus != NULL)        gst_object_unref(bus);

    g_print("Try to end pipeline\n");
	if (pipeline != NULL)	{ gst_element_set_state(pipeline, GST_STATE_NULL); gst_object_unref(pipeline);}

	return 0;
};

