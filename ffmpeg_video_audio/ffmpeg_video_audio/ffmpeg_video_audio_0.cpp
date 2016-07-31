// ffmpeg_video_audio.cpp : �������̨Ӧ�ó������ڵ㡣
//ffmpeg������Ƶ�ļ�,û����Ƶ����

#include "stdafx.h"
#include "ffmpeg_video_audio.h"

int main(int argc, char* argv[])
{
	AVFormatContext *pFormatCtx = NULL;
	int i, index_video = -1;
	AVCodecContext  *pCodecCtx = NULL;
	AVCodec         *pCodec = NULL;
	AVFrame         *pFrame = NULL, *pFrameYUV = NULL;
	uint8_t         *out_buffer = NULL;
	AVPacket        *pkt;
	int             frameFinished;
	struct SwsContext *img_convert_ctx = NULL;

	//SDL2.0
	SDL_Window      *screen;
	SDL_Renderer    *sdlrenderer;
	SDL_Texture     *sdltexture;
	SDL_Rect        sdlrect;
	SDL_Event       event;


	char *filename = "forrest.mp4";

	// Register all formats and codecs
	av_register_all();
	avformat_network_init();

	pFormatCtx = avformat_alloc_context(); //�����ڴ�
	if (avformat_open_input(&pFormatCtx, filename, NULL, NULL) < 0) //��������Ƶ�ļ�
	{
		printf("Can't open the input stream.\n");
		return -1;
	}
	if (avformat_find_stream_info(pFormatCtx, NULL)<0)     //����Ƶ�ļ��еõ���ý����Ϣ
	{
		printf("Can't find the stream information!\n");
		return -1;
	}
	//��������Ϣ
	av_dump_format(pFormatCtx, 0, filename, 0);
	
	//���ҵ�һ����Ƶ������
	for (i = 0; i < pFormatCtx->nb_streams; i++)
	{
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)      //�������Ƶ�������¼��������
		{
			index_video = i;
			break;
		}
	}
	if (index_video == -1)
	{
		printf("Can't find a video stream;\n");
		return -1;
	}

	//���ҽ�������Ϣ
	pCodecCtx = pFormatCtx->streams[index_video]->codec;
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);     //���ҽ�����
	if (pCodec == NULL)
	{
		printf("Can't find a decoder!\n");
		return -1;
	}

	//�򿪱�����
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
	{
		printf("Can't open the decoder!\n");
		return -1;
	}

	//�洢���������
	pFrame = av_frame_alloc();  //this only allocates the AVFrame itself, not the data buffers
	pFrameYUV = av_frame_alloc();
	out_buffer = (uint8_t *)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height));  //���ٻ�����
	avpicture_fill((AVPicture *)pFrameYUV, out_buffer, AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);//֡��������ڴ���

	//�洢����ǰ����
	pkt = (AVPacket *)av_malloc(sizeof(AVPacket));;
	av_init_packet(pkt);
	img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
		pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
	
	//SDL2.0
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
		fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
		exit(1);
	}
	int screen_w = pCodecCtx->width, screen_h = pCodecCtx->height;
	screen = SDL_CreateWindow("SDL EVENT TEST", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screen_w, screen_h, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE); //������ʾ����
	if (screen == NULL)
	{
		printf("Can't creat a window:%s\n", SDL_GetError());
		return -1;
	}
	sdlrenderer = SDL_CreateRenderer(screen, -1, 0);//������Ⱦ��
	sdltexture = SDL_CreateTexture(sdlrenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, pCodecCtx->width, pCodecCtx->height);//��������


	while (av_read_frame(pFormatCtx, pkt) >= 0) {
		
		// �ж��Ƿ�����Ƶ��
		if (pkt->stream_index == index_video) {
			// ������Ƶ������
			avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished,pkt);

			// �õ�֡����
			if (frameFinished) {
				sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height,
					pFrameYUV->data, pFrameYUV->linesize);
				
				SDL_UpdateTexture(sdltexture, NULL, pFrameYUV->data[0], pFrameYUV->linesize[0]); //�������������

				//���ô��ڴ�С
				sdlrect.x = 0;
				sdlrect.y = 0;
				sdlrect.w = screen_w;
				sdlrect.h = screen_h;

				SDL_RenderCopy(sdlrenderer, sdltexture, NULL, &sdlrect); //����������Ϣ����Ⱦ��Ŀ��
				SDL_RenderPresent(sdlrenderer);//��Ƶ��Ⱦ��ʾ
				SDL_Delay(40);
			}
		}

		// Free the packet that was allocated by av_read_frame
		av_free_packet(pkt);
		SDL_PollEvent(&event);

		switch (event.type) {
		case SDL_QUIT:
			SDL_Quit();
			exit(0);
			break;
		default:
			break;
		}

	}
	sws_freeContext(img_convert_ctx);
	// Free the YUV frame
	av_free(pFrame);
	av_frame_free(&pFrameYUV);
	// Close the codec
	avcodec_close(pCodecCtx);

	// Close the video file
	avformat_close_input(&pFormatCtx);

	return 0;
}

