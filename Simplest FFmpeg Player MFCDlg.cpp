
// Simplest FFmpeg Player MFCDlg.cpp : ʵ���ļ�
//

/**
* ��򵥵�ͼ�ν������Ƶ������
* Simplest FFmpeg Player GUI
*
* ԭ����
* ������ Lei Xiaohua
* leixiaohua1020@126.com
* �й���ý��ѧ/���ֵ��Ӽ���
* Communication University of China / Digital TV Technology
* http://blog.csdn.net/leixiaohua1020
*
* �޸ģ�
* ���ĳ� Liu Wenchen
* 812288728@qq.com
* ���ӿƼ���ѧ/������Ϣ
* University of Electronic Science and Technology of China / Electronic and Information Science
* https://blog.csdn.net/ProgramNovice
*/

#include "stdafx.h"
#include "Simplest FFmpeg Player MFC.h"
#include "Simplest FFmpeg Player MFCDlg.h"
#include "afxdialogex.h"

#pragma warning(disable:4996)

#define __STDC_CONSTANT_MACROS

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "SDL2/SDL.h"
};

// ����
// LNK2019 �޷��������ⲿ���� __imp__fprintf���÷����ں��� _ShowError �б�����
// LNK2019 �޷��������ⲿ���� __imp____iob_func���÷����ں��� _ShowError �б�����

// ����취��
// ������ı������汾���ڵ�ǰ����汾����Ҫ��������Դ����vs2017���±��룬����û�а������Դ�룬��·��ͨ��
// Ȼ��鵽˵��stdin, stderr, stdout �⼸������vs2015����ǰ�Ķ���ò�һ�������Ա���
// ��������أ�����ʹ��{ *stdin,*stdout,*stderr }�����Լ�����__iob_func()
#pragma comment(lib,"legacy_stdio_definitions.lib")
extern "C"
{
	FILE __iob_func[3] = { *stdin,*stdout,*stderr };
}

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���
class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

	// �Ի�������
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

														// ʵ��
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CSimplestFFmpegPlayerMFCDlg �Ի���
CSimplestFFmpegPlayerMFCDlg::CSimplestFFmpegPlayerMFCDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_SIMPLESTFFMPEGPLAYERMFC_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CSimplestFFmpegPlayerMFCDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_URL, m_url);
}

BEGIN_MESSAGE_MAP(CSimplestFFmpegPlayerMFCDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_ABOUT, &CSimplestFFmpegPlayerMFCDlg::OnBnClickedAbout)
	ON_BN_CLICKED(IDC_FILEDIALOG, &CSimplestFFmpegPlayerMFCDlg::OnBnClickedFiledialog)
	ON_BN_CLICKED(IDC_PLAY, &CSimplestFFmpegPlayerMFCDlg::OnBnClickedPlay)
	ON_BN_CLICKED(IDC_PAUSE, &CSimplestFFmpegPlayerMFCDlg::OnBnClickedPause)
	ON_BN_CLICKED(IDC_STOP, &CSimplestFFmpegPlayerMFCDlg::OnBnClickedStop)
	ON_BN_CLICKED(IDC_CLOSE, &CSimplestFFmpegPlayerMFCDlg::OnBnClickedClose)
END_MESSAGE_MAP()

// �Զ�����Ϣ����
#define REFRESH_EVENT  (SDL_USEREVENT + 1) // Refresh Event
#define BREAK_EVENT  (SDL_USEREVENT + 2) // Break

// �̱߳�־λ
int thread_exit = 0;// �˳���־������1���˳�
int thread_pause = 0;// ��ͣ��־������1����ͣ

// ��Ƶ������ز���
int delay_time = 40;
// bool video_gray = false;

// ����ˢ���߳�
int refresh_video(void *opaque)
{
	thread_exit = 0;
	thread_pause = 0;
	while (thread_exit == 0)
	{
		if (thread_pause == 0)
		{
			SDL_Event event;
			event.type = REFRESH_EVENT;
			// �����̷߳���ˢ���¼�
			SDL_PushEvent(&event);
		}
		// ���ߺ�����������ʱ
		SDL_Delay(delay_time);
	}
	// ��Ҫ��������
	SDL_Event event;
	event.type = BREAK_EVENT;
	// �����̷߳����˳�ѭ���¼�
	SDL_PushEvent(&event);
	thread_exit = 0;
	thread_pause = 0;
	return 0;
}

UINT ffmpegPlayer(LPVOID lpParam)
{
	AVFormatContext	*pFormatCtx;
	int				i, videoindex;
	AVCodecContext	*pCodecCtx;
	AVCodec			*pCodec;
	AVFrame	*pFrame, *pFrameYUV;
	uint8_t *out_buffer;
	AVPacket *packet;
	int y_size;
	int ret, got_picture;
	//------------ SDL ----------------
	int screen_w, screen_h;
	SDL_Window *screen;// ����
	SDL_Renderer* sdlRenderer;// ��Ⱦ��
	SDL_Texture* sdlTexture;// ����
	SDL_Rect sdlRect;// ��Ⱦ��ʾ���
	SDL_Thread *refresh_thread;// ����ˢ���߳�
	SDL_Event event;// ���߳�ʹ�õ��¼�

	struct SwsContext *img_convert_ctx;
	//------------MFC----------------
	CSimplestFFmpegPlayerMFCDlg *sfp_dlg = (CSimplestFFmpegPlayerMFCDlg *)lpParam;
	char filepath[250] = { 0 };// �����ļ�·��
	GetWindowText(sfp_dlg->m_url, (LPSTR)filepath, 250);
	//----------MFC End--------------
	int frame_cnt;

	av_register_all();
	avformat_network_init();
	// ����avFormatContext�ռ䣬�ǵ�Ҫ�ͷ�
	pFormatCtx = avformat_alloc_context();
	// ��ý���ļ�
	if (avformat_open_input(&pFormatCtx, filepath, NULL, NULL) != 0)
	{
		printf("Couldn't open input stream.\n");
		return -1;
	}
	// ��ȡý���ļ���Ϣ����pFormatCtx��ֵ
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
	{
		printf("Couldn't find stream information.\n");
		return -1;
	}
	videoindex = -1;
	for (i = 0; i < pFormatCtx->nb_streams; i++)
	{
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			videoindex = i;
			break;
		}
	}
	if (videoindex == -1)
	{
		printf("Didn't find a video stream.\n");
		return -1;
	}
	pCodecCtx = pFormatCtx->streams[videoindex]->codec;
	// ������Ƶ����Ϣ��codec_id�ҵ���Ӧ�Ľ�����
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodec == NULL)
	{
		printf("Codec not found.\n");
		return -1;
	}
	// ʹ�ø�����pCodec��ʼ��pCodecCtx
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
	{
		printf("Could not open codec.\n");
		return -1;
	}

	/*
	* �ڴ˴���������Ƶ��Ϣ�Ĵ���
	* ȡ����pFormatCtx��ʹ��fprintf()
	*/
	FILE *fp_txt = fopen("info.txt", "wb+");

	fprintf(fp_txt, "��װ��ʽ��%s\n", pFormatCtx->iformat->long_name);
	fprintf(fp_txt, "�����ʣ�%d\n", pFormatCtx->bit_rate);
	fprintf(fp_txt, "��Ƶʱ����%d\n", pFormatCtx->duration);
	fprintf(fp_txt, "��Ƶ���뷽ʽ��%s\n", pFormatCtx->streams[videoindex]->codec->codec->long_name);
	fprintf(fp_txt, "��Ƶ�ֱ��ʣ�%d * %d\n", pFormatCtx->streams[videoindex]->codec->width, pFormatCtx->streams[videoindex]->codec->height);

	// ��avcodec_receive_frame()������Ϊ��������ȡ��frame
	// ��ȡ����frame��Щ�����Ǵ����Ҫ���˵���������Ӧ֡���ܳ�������
	pFrame = av_frame_alloc();
	//��Ϊyuv�����frame�����ߣ���������ź͹��˳����֡��YUV��Ӧ������Ҳ�ǴӸö����ж�ȡ
	pFrameYUV = av_frame_alloc();
	// ������Ⱦ�����ݣ��Ҹ�ʽΪYUV420P
	out_buffer = (uint8_t *)av_malloc(avpicture_get_size(PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height));
	avpicture_fill((AVPicture *)pFrameYUV, out_buffer, PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);

	// Output Info
	printf("--------------- File Information ----------------\n");
	av_dump_format(pFormatCtx, 0, filepath, 0);
	printf("-------------------------------------------------\n");
	// ���ڽ��������֡��ʽ��һ����YUV420P��,����Ⱦ֮ǰ��Ҫ���и�ʽת��
	img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
		pCodecCtx->width, pCodecCtx->height, PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

	// ֡������
	frame_cnt = 0;

	// ---------------------- SDL ----------------------
	// ��ʼ��SDLϵͳ
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER))
	{
		printf("Couldn't initialize SDL - %s\n", SDL_GetError());
		return -1;
	}
	// SDL 2.0 Support for multiple windows
	screen_w = pCodecCtx->width;
	screen_h = pCodecCtx->height;
	// ��Ƶ���صĿ�͸�
	const int pixel_w = screen_w;
	const int pixel_h = screen_h;
	// ��������SDL_Window����ʾ�ڵ������ڣ�
	// screen = SDL_CreateWindow("Simplest FFmpeg Player", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screen_w, screen_h, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	//��ʾ��MFC�ؼ���
	screen = SDL_CreateWindowFrom(sfp_dlg->GetDlgItem(IDC_SCREEN)->GetSafeHwnd());
	if (!screen)
	{
		printf("SDL: Couldn't create window - exiting:%s\n", SDL_GetError());
		return -1;
	}
	// ������Ⱦ��SDL_Renderer
	sdlRenderer = SDL_CreateRenderer(screen, -1, 0);

	Uint32 pixformat = SDL_PIXELFORMAT_IYUV;
	// IYUV: Y + U + V  (3 planes)
	// YV12: Y + V + U  (3 planes)
	// ��������SDL_Texture
	sdlTexture = SDL_CreateTexture(sdlRenderer, pixformat, SDL_TEXTUREACCESS_STREAMING, pCodecCtx->width, pCodecCtx->height);

	sdlRect.x = 0;
	sdlRect.y = 0;
	sdlRect.w = screen_w;
	sdlRect.h = screen_h;

	packet = (AVPacket *)av_malloc(sizeof(AVPacket));
	// ��������ˢ���߳�
	refresh_thread = SDL_CreateThread(refresh_video, NULL, NULL);
	// ---------------------- SDL End ----------------------
	// ��ʼһ֡һ֡��ȡ
	while (1)
	{
		// Wait
		SDL_WaitEvent(&event);// ���¼�������ȡ�¼�
		if (event.type == REFRESH_EVENT)
		{
			while (1)
			{
				if (av_read_frame(pFormatCtx, packet) < 0)
				{
					thread_exit = 1;
				}
				if (packet->stream_index == videoindex)
				{
					break;
				}
			}

			// ���ÿһ������ǰ��Ƶ֡������֡��С
			fprintf(fp_txt, "֡%d��С��%d\n", frame_cnt, packet->size);

			ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
			if (ret < 0)
			{
				printf("Decode Error.\n");
				return -1;
			}
			if (got_picture)
			{
				// ��ʽת�������������ݾ���sws_scale()��������ȥ����Ч����
				sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height,
					pFrameYUV->data, pFrameYUV->linesize);
				printf("Decoded frame index: %d\n", frame_cnt);

				// ���ÿһ���������Ƶ֡������֡����
				char pict_type_str[10];
				switch (pFrame->pict_type)
				{
				case AV_PICTURE_TYPE_NONE:
					sprintf(pict_type_str, "NONE");
					break;
				case AV_PICTURE_TYPE_I:
					sprintf(pict_type_str, "I");
					break;
				case AV_PICTURE_TYPE_P:
					sprintf(pict_type_str, "P");
					break;
				case AV_PICTURE_TYPE_B:
					sprintf(pict_type_str, "B");
					break;
				case AV_PICTURE_TYPE_SI:
					sprintf(pict_type_str, "SI");
					break;
				case AV_PICTURE_TYPE_S:
					sprintf(pict_type_str, "S");
					break;
				case AV_PICTURE_TYPE_SP:
					sprintf(pict_type_str, "SP");
					break;
				case AV_PICTURE_TYPE_BI:
					sprintf(pict_type_str, "BI");
					break;
				default:
					break;
				}
				fprintf(fp_txt, "֡%d���ͣ�%s\n", frame_cnt, pict_type_str);

				// ��ѡ����ʾ�ڰ�ͼ����buffer��U��V��������Ϊ128
				//if (video_gray == true)
				//{
				//	// U��V��ͼ���еľ���ƫ�ô����ɫ�ȷ���
				//	// ��ƫ�ô���ǰ������ȡֵ��Χ��-128-127����ʱ����U��V�����޸�Ϊ0������ɫ
				//	// ��ƫ�ô��������ȡֵ��Χ�����0-255��������ʱ����Ҫȡ�м�ֵ����128
				//	memset(pFrameYUV->data[0] + pixel_w * pixel_h, 128, pixel_w * pixel_h / 2);
				//}
				// �������������
				SDL_UpdateTexture(sdlTexture, NULL, pFrameYUV->data[0], pFrameYUV->linesize[0]);
				// FIX: If window is resize
				sdlRect.x = 0;
				sdlRect.y = 0;
				sdlRect.w = screen_w;
				sdlRect.h = screen_h;

				// ʹ��ͼ����ɫ�����ǰ����ȾĿ��
				SDL_RenderClear(sdlRenderer);
				// ����������ݿ�������Ⱦ��
				SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, &sdlRect);
				// ��ʾ���������
				SDL_RenderPresent(sdlRenderer);

				frame_cnt++;
			}
			av_free_packet(packet);
		}
		else if (event.type == SDL_KEYDOWN)
		{
			// ���ݰ��¼��̼�λ�����¼�
			switch (event.key.keysym.sym)
			{
			case SDLK_ESCAPE:
				thread_exit = 1;// ����ESC����ֱ���˳�������
				break;
			case SDLK_SPACE:
				thread_pause = !thread_pause;// ����Space����������Ƶ������ͣ
				break;
			//case SDLK_F1:
			//	delay_time += 10;// ����F1����Ƶ����
			//	break;
			//case SDLK_F2:
			//	if (delay_time > 10)
			//	{
			//		delay_time -= 10;// ����F2����Ƶ����
			//	}
			//	break;
			//case SDLK_LSHIFT:
			//	video_gray = !video_gray;// ������Shift�����л���ʾ��ɫ/�ڰ�ͼ��
			//	break;
			default:
				break;
			}
		}
		else if (event.type == SDL_QUIT)
		{
			thread_exit = 1;
		}
		else if (event.type == BREAK_EVENT)
		{
			break;
		}
	}

	// �ر��ļ�
	fclose(fp_txt);

	// �ͷ�FFmpeg�����Դ
	sws_freeContext(img_convert_ctx);
	// �ͷ�SDL��Դ
	SDL_DestroyTexture(sdlTexture);
	SDL_DestroyRenderer(sdlRenderer);
	SDL_DestroyWindow(screen);
	// �˳�SDLϵͳ
	SDL_Quit();

	//FIX Small Bug
	//SDL Hide Window When it finished
	sfp_dlg->GetDlgItem(IDC_SCREEN)->ShowWindow(SW_SHOWNORMAL);

	av_frame_free(&pFrameYUV);
	av_frame_free(&pFrame);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);

	return 0;
}

// CSimplestFFmpegPlayerMFCDlg ��Ϣ�������

BOOL CSimplestFFmpegPlayerMFCDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// ��������...���˵�����ӵ�ϵͳ�˵��С�

	// IDM_ABOUTBOX ������ϵͳ���Χ�ڡ�
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// ���ô˶Ի����ͼ�ꡣ  ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

									// TODO: �ڴ���Ӷ���ĳ�ʼ������

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

void CSimplestFFmpegPlayerMFCDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ  ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CSimplestFFmpegPlayerMFCDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR CSimplestFFmpegPlayerMFCDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CSimplestFFmpegPlayerMFCDlg::OnBnClickedAbout()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	CAboutDlg about_dlg;
	about_dlg.DoModal();
}


void CSimplestFFmpegPlayerMFCDlg::OnBnClickedFiledialog()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	CString file_path_name;
	CFileDialog file_dlg(TRUE, NULL, NULL, NULL, NULL);//TRUEΪOPEN�Ի���FALSEΪSAVE AS�Ի��� 
	if (file_dlg.DoModal() == IDOK)
	{
		file_path_name = file_dlg.GetPathName();
		m_url.SetWindowText(file_path_name);
	}
}

UINT Thread_Play(LPVOID lpParam)
{
	CSimplestFFmpegPlayerMFCDlg *sfp_dlg = (CSimplestFFmpegPlayerMFCDlg *)lpParam;
	ffmpegPlayer(lpParam);
	return 0;
}

void CSimplestFFmpegPlayerMFCDlg::OnBnClickedPlay()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	CString str;
	m_url.GetWindowText(str);
	if (str.IsEmpty())
	{
		AfxMessageBox("�������ļ�·����");
		return;
	}
	AfxBeginThread(Thread_Play, this);//�����߳�
}


void CSimplestFFmpegPlayerMFCDlg::OnBnClickedPause()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	thread_pause = !thread_pause;
}


void CSimplestFFmpegPlayerMFCDlg::OnBnClickedStop()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	thread_exit = 1;
}


void CSimplestFFmpegPlayerMFCDlg::OnBnClickedClose()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	CDialogEx::OnCancel();
}
