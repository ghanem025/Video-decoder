//Name: Ghanem Ghanem
//ID: 110005430
//Date: 2023-02-05

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <stdint.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/avutil.h>
#include <pulse/simple.h>
#include <pulse/error.h>
#include <libswresample/swresample.h>
#include <pulse/pulseaudio.h>


#define INBUF_SIZE 4096

#define FRAME_BUFFER_SIZE 1000

AVFrame* buffer[FRAME_BUFFER_SIZE];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condition = PTHREAD_COND_INITIALIZER;


unsigned int r=0, w=0;
int framerate = 30;
AVFrame *Free_frame = NULL;
pa_mainloop *mainloop;

struct decode_video_args{
	int argc;
	char **argv;
};

struct decode_video_args decode_args;

static void yuvToRgbFrame(AVFrame *inputFrame, AVFrame *outputFrame) {
	struct SwsContext *sws_ctx = sws_getContext(
	    inputFrame->width, inputFrame->height, inputFrame->format,
	    inputFrame->width, inputFrame->height, AV_PIX_FMT_RGB32, SWS_BICUBIC,
	    NULL, NULL, NULL);

	outputFrame->format = AV_PIX_FMT_RGB32;
	outputFrame->width = inputFrame->width;
	outputFrame->height = inputFrame->height;
	av_frame_get_buffer(outputFrame, 32);

	sws_scale(sws_ctx, (const uint8_t *const *)inputFrame->data,
	          inputFrame->linesize, 0, inputFrame->height,
	          outputFrame->data, outputFrame->linesize);
}

static void decode(AVCodecContext *dec_ctx, AVFrame *frame, AVPacket *pkt)
{
	char buf[1024];
	int ret;

	ret = avcodec_send_packet(dec_ctx, pkt);
	if (ret < 0) {
		fprintf(stderr, "Error sending a packet for decoding\n");
		exit(1);
	}

	while (ret >= 0) {
		AVFrame *decode_frame = av_frame_alloc();
		ret = avcodec_receive_frame(dec_ctx, frame);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
			return ;
		}
		else if (ret < 0) {
			fprintf(stderr, "Error during decoding\n");
			exit(1);
		}
        // printf("saving frame %3d\n", dec_ctx->frame_number);
		yuvToRgbFrame(frame, decode_frame);
		pthread_mutex_lock(&mutex);
		if(w == r + FRAME_BUFFER_SIZE){
			pthread_cond_wait(&condition, &mutex);
		}

		buffer[(w++) % FRAME_BUFFER_SIZE] = decode_frame;

		pthread_cond_signal(&condition);
		pthread_mutex_unlock(&mutex);

	}
}


int decode_video(int argc, char **argv, int pas)
{

	const char *filename, *outfilename;
	const AVCodec *codec;
	const char *output_dir;
	AVFormatContext *pFormatCtx = NULL;
	AVCodecParameters    *pCodecCtxOrig = NULL;
	AVCodecContext* pCodecCtx = NULL;
	FILE *f;
	AVFrame *frame;
	uint8_t inbuf[INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
	int frame_number;
	int ret;
	AVPacket *pkt;
	int result;
	bool thing = true;
	struct stat st = {0};

    const AVCodec *audio_codec;
    AVFormatContext *input_ctx = NULL;
    AVCodecContext *audio_codec_ctx = NULL, *video_codec_ctx = NULL;

	if (argc <= 2) {
		fprintf(stderr, "Usage: %s <input file> <output file>\n"
		        "And check your input file is encoded by mpeg1video please.\n", argv[0]);
		exit(0);
	}
	filename = argv[1];

	if (stat(output_dir, &st) == -1) {
		mkdir(output_dir, 0700);
	}

	/* set end of buffer to 0 (this ensures that no overreading happens for damaged MPEG streams) */
	memset(inbuf + INBUF_SIZE, 0, AV_INPUT_BUFFER_PADDING_SIZE);

	if (avformat_open_input(&pFormatCtx, argv[1], NULL, NULL) != 0){
		fprintf(stderr, "FFmpeg failed to open file %s!\n", filename);
		exit(-1);
	}

	if (avformat_find_stream_info(pFormatCtx, NULL) < 0){
		fprintf(stderr, "FFmpeg failed to retrieve stream info!\n");
		exit(-1);

	}

    int audio_stream_index = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, &audio_codec, 0);
    int video_stream_index = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);

    audio_codec_ctx = avcodec_alloc_context3(audio_codec);
    avcodec_parameters_to_context(audio_codec_ctx, pFormatCtx->streams[audio_stream_index]->codecpar);
    avcodec_open2(audio_codec_ctx, audio_codec, NULL);

	av_dump_format(pFormatCtx, 0, argv[1], 0);

	int videoStream=-1;
	for(int i=0; i<pFormatCtx->nb_streams; i++)
		if(pFormatCtx->streams[i]->codecpar->codec_type==AVMEDIA_TYPE_VIDEO) {
			videoStream=i;
			break;
		}
	if(videoStream==-1)
		return -1; // Didn't find a video stream
  
	pCodecCtxOrig = pFormatCtx->streams[videoStream]->codecpar;

	codec = avcodec_find_decoder(pCodecCtxOrig->codec_id);
	if (!codec) {
		fprintf(stderr, "Codec not found\n");
		exit(1);
	}
	if(codec == NULL){
		fprintf(stderr, "Unsupported codec!\n");
		return -1; // Codec not found
	}

	pCodecCtx = avcodec_alloc_context3(codec);
	if(!pCodecCtx){
		fprintf(stderr, "not allocated properly");
		return -1;
	}
	ret = avcodec_parameters_to_context(pCodecCtx, pCodecCtxOrig);
	if(ret < 0){
		fprintf(stderr, "Couldn't copy codec context");
		return -1; // Error copying codec context
	}

	if (avcodec_open2(pCodecCtx, codec, NULL) < 0) {
		fprintf(stderr, "Could not open codec\n");
		exit(1);
	}
    int err;

	pa_simple *s;
	pa_sample_spec ss;
	
	ss.format = PA_SAMPLE_FLOAT32;
	ss.channels = 1;
	ss.rate = audio_codec_ctx->sample_rate;
	int pa_error =0;
    s = pa_simple_new(NULL, "ffmpeg", PA_STREAM_PLAYBACK, NULL, "playback", &ss, NULL, NULL, NULL);

    if (s == NULL) {
		fprintf(stderr, "Error: %s\n", pa_strerror(pa_error));
		exit(1);
    }

	frame = av_frame_alloc();
	if (!frame) {
		fprintf(stderr, "Could not allocate video frame\n");
		exit(1);
	}

	AVPacket *packet = av_packet_alloc();
	while(av_read_frame(pFormatCtx, packet)>=0) {

		if(packet->stream_index==videoStream) {
			decode(pCodecCtx, frame, packet);
		}
		if (packet->stream_index == audio_stream_index) {
            avcodec_send_packet(audio_codec_ctx, packet);
            while ((ret = avcodec_receive_frame(audio_codec_ctx, frame)) == 0) {
				pa_simple_write(s, frame->data[0], frame->linesize[0]/2, NULL);
            }
        }

	}

    avcodec_flush_buffers(audio_codec_ctx);
	avcodec_close(pCodecCtx);
	av_frame_free(&frame);
	av_packet_free(&packet);
    avcodec_free_context(&audio_codec_ctx);
    avformat_close_input(&input_ctx);


	return 0;
}

void *decode_video_thread(void *args){
	struct decode_video_args *decode_args = (struct decode_video_args *)args;
	printf("Decode Thread running:");
	decode_video(decode_args->argc, decode_args->argv, 1);
	printf("exiting decode thread\n");
	pthread_exit(NULL);
}
void Garbage_collector(AVFrame *read_frame){
	if (Free_frame!= NULL) {
		av_frame_free(&Free_frame);
	}
	Free_frame = read_frame;
}

void draw(GtkDrawingArea *area, cairo_t *cr, int width, int height,
          gpointer user_data){
	AVFrame *read_frame = NULL;
	pthread_mutex_lock(&mutex);
	if(r == w){
		puts("Buffer is empty");
		pthread_cond_wait(&condition, &mutex);
	}

	read_frame = buffer[(r++) % FRAME_BUFFER_SIZE];

	pthread_cond_signal(&condition);
	pthread_mutex_unlock(&mutex);

	if(read_frame !=NULL){
		cairo_surface_t *frameSurface =
		    cairo_image_surface_create_for_data(
			read_frame->data[0], CAIRO_FORMAT_ARGB32, read_frame->width,
			read_frame->height, read_frame->linesize[0]);

		cairo_set_source_surface(cr, frameSurface, 0, 0);
		cairo_paint(cr);
		Garbage_collector(read_frame);
	}
}

gboolean update_ui(GtkWidget *area) {
	gtk_widget_queue_draw(area);
	return FALSE;
}
void *display_frame_thread(void *args){
	while (1) {
		g_idle_add((GSourceFunc)update_ui, args);
		usleep(1000000 / framerate);
	}
	printf("Thread is exiting...\n");
	pthread_exit(NULL);
}
static void on_window_closed(GtkWindow *window, gpointer user_data)
{
	puts("closing window...\n\n");
	gtk_window_destroy(GTK_WINDOW(window));
}

void activate(GtkApplication *app, gpointer user_data) {

	AVFormatContext *pFormatCtx;
	AVCodecContext *pCodecCtx;
	const AVCodec *pCodec;
	AVPacket *pkt;
	AVFrame *pFrame;

	const char *filename = (void *) user_data;
		// Open the input file
	printf("in activate %s\n", filename);
    avformat_open_input(&pFormatCtx, filename, NULL, NULL);
    // Get the codec parameters
    avformat_find_stream_info(pFormatCtx, NULL);

    // Find the video stream
	pCodec = NULL;
	int videoStream = av_find_best_stream(
			pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, &pCodec, 0);

	pCodecCtx = avcodec_alloc_context3(pCodec);
    avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[videoStream]->codecpar);

    avcodec_open2(pCodecCtx, pCodec, NULL);




	GtkWidget *window, *box, *drawingArea;
	window = gtk_application_window_new(app);

	gtk_widget_set_name(window, "parent");
	box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	drawingArea = gtk_drawing_area_new();

	gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(drawingArea),
	                               draw, NULL,
	                               NULL);
	gtk_widget_set_size_request(drawingArea, pCodecCtx->width, pCodecCtx->height);

	pthread_t thread_id;
	pthread_create(&thread_id, NULL,decode_video_thread, (void *)&decode_args);

	pthread_t thread_id2;
	pthread_create(&thread_id2, NULL, display_frame_thread, (void *)drawingArea);


	gtk_box_append(GTK_BOX(box), drawingArea);
	g_signal_connect(window, "destroy", G_CALLBACK(on_window_closed), app);
	gtk_window_set_child(GTK_WINDOW(window), box);
	gtk_window_set_title(GTK_WINDOW(window),
	                     "Main application window");
	gtk_widget_show(window);
}


int main(int argc, char *argv[]) {
	if(argc < 2){
		printf("You must pass in two arguments, the first "
		    "argument is a video file, the second argument is an "
		    "integer (framerate)");
		return 1;
	}

	decode_args.argc = argc;
	decode_args.argv = argv;
	framerate = atoi(argv[2]);

	GtkApplication *app;
	int status;
	app = gtk_application_new("org.A2.example",
	                          G_APPLICATION_DEFAULT_FLAGS);

	g_signal_connect(app, "activate", G_CALLBACK(activate), (void * ) argv[1]);
	status = g_application_run(G_APPLICATION(app), 1, argv);
	g_object_unref(app);



	return status;

}
