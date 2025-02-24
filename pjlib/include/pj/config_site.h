 //启用视频
#define PJSUA_HAS_VIDEO 0

//启用视频
#define PJMEDIA_HAS_VIDEO 0

 //启用SDL视频设备，否则无法显示视频
#define PJMEDIA_VIDEO_DEV_HAS_SDL 0

 //不依赖OPENGL
#define PJMEDIA_VIDEO_DEV_SDL_HAS_OPENGL 0

 //不使用ffmepg视频设备，wpf的设备过时了
#define PJMEDIA_VIDEO_DEV_HAS_FFMPEG 0

 //启用ffmpeg，需要用到h264的codec进行视频编解码
#define PJMEDIA_HAS_FFMPEG 0

 //启用h264,不起用，microsip的setting
#define PJMEDIA_HAS_FFMPEG_CODEC_H264 0

//是否应该添加 ICE 媒体功能的标签参数
#define PJSUA_ADD_ICE_TAGS 0

#define PJMEDIA_HAS_BCG729 1

#define PJMEDIA_HAS_WEBRTC_AEC 1
