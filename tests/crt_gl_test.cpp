// Active desktop shader regression: real GL compilation and framebuffer readback.
// Run from a graphical session; no game assets required.
#include "../PORT/MAC/src/ra_crt_gl.cpp"
#include <math.h>
#include <stdio.h>
#include <vector>
#include <algorithm>

static double linear(unsigned char c)
{
 double v = c / 255.0;
 return v <= 0.04045 ? v / 12.92 : pow((v + 0.055) / 1.055, 2.4);
}

static void redraw()
{
 pglUseProgram(CRTProgram);
 glBegin(GL_QUADS);
 glTexCoord2f(0,0);glVertex2f(-1,1); glTexCoord2f(1,0);glVertex2f(1,1);
 glTexCoord2f(1,1);glVertex2f(1,-1); glTexCoord2f(0,1);glVertex2f(-1,-1);
 glEnd();
}

static bool save_pixels(char const *path, int w, int h, std::vector<unsigned char> &pixels)
{
 SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormatFrom(&pixels[0],w,h,32,w*4,SDL_PIXELFORMAT_RGBA32);
 bool saved = surface && SDL_SaveBMP(surface,path) == 0;
 if (surface) SDL_FreeSurface(surface);
 return saved;
}

static int glow_checks(SDL_Window *window, char const *art_path)
{
 int heights[] = {1080, 1440, 2160, 937};
 char const *amounts[] = {"0", "25", "100"};
 std::vector<uint32_t> source(856*400);
 for (int y=0;y<400;++y) for (int x=0;x<856;++x)
  source[y*856+x] = (x%47 < 2 && y%31 < 3) ? 0xFFF8E8A0U : ((x/7+y/5)%2 ? 0xFF405030U : 0xFF182820U);
 if(art_path) {
  SDL_Surface *loaded=SDL_LoadBMP(art_path);
  SDL_Surface *art=loaded?SDL_ConvertSurfaceFormat(loaded,SDL_PIXELFORMAT_ARGB8888,0):0;
  if(loaded) SDL_FreeSurface(loaded);
  if(!art || art->w!=856 || art->h!=400) return 17;
  for(int y=0;y<400;++y) memcpy(&source[y*856],(char *)art->pixels+y*art->pitch,856*4);
  SDL_FreeSurface(art);
 }
 printf("GPU %s; GL %s\n",glGetString(GL_RENDERER),glGetString(GL_VERSION));
 for (int size=0;size<4;++size) {
  int h=heights[size],w=h*16/9;
  SDL_SetWindowSize(window,w,h);
  RA_CRTGL_GetOutputSize(&w,&h);
  RAAspectViewport viewport={0,0,w,h};
  for (int setting=0;setting<3;++setting) {
   setenv("RA_CRT_GLOW",amounts[setting],1);
   std::vector<double> times;
   RACRTGLConfig config={25,0,0,0,0};
   for (int frame=0;frame<35;++frame) {
    glFinish();
    Uint64 start=SDL_GetPerformanceCounter();
    if (!RA_CRTGL_Present(&source[0],856,400,856,viewport,config)) return 10;
    glFinish();
    double ms=1000.0*(SDL_GetPerformanceCounter()-start)/SDL_GetPerformanceFrequency();
    if (frame>=5) times.push_back(ms);
   }
   if(art_path && size==0 && setting<2) {
    std::vector<unsigned char> pixels(w*h*4);
    redraw();glReadBuffer(GL_BACK);
    glReadPixels(0,0,w,h,GL_RGBA,GL_UNSIGNED_BYTE,&pixels[0]);
    if(!save_pixels(setting?"build/crt-review/art-glow-25.bmp":"build/crt-review/art-glow-off.bmp",w,h,pixels)) return 18;
   }
   std::sort(times.begin(),times.end());
   int over=0;
   for (size_t i=0;i<times.size();++i) if(times[i]>1000.0/60) ++over;
   printf("BENCH %dx%d glow=%s present+finish median=%.2fms p95=%.2fms over16.67=%d/30\n",w,h,amounts[setting],times[15],times[28],over);
  }
 }
 // A single white emitter must add light in all four directions, locally.
 int w=1920,h=1080;
 SDL_SetWindowSize(window,w,h);
 RA_CRTGL_GetOutputSize(&w,&h);
 RAAspectViewport viewport={0,0,w,h};
 std::fill(source.begin(),source.end(),0xFF000000U);
 source[200*856+428]=0xFFFFFFFFU;
 std::vector<unsigned char> off(w*h*4),on(w*h*4);
 for(int scene=0;scene<2;++scene) {
  if(scene==1) std::fill(source.begin(),source.end(),0xFF404040U);
  for(int setting=0;setting<2;++setting) {
   setenv("RA_CRT_GLOW",amounts[setting],1);
   RACRTGLConfig config={0,0,0,0,0};
   if(!RA_CRTGL_Present(&source[0],856,400,856,viewport,config)) return 11;
   // Render to back buffer for portable hidden-window readback.
   redraw();
   glReadBuffer(GL_BACK);
   glReadPixels(0,0,w,h,GL_RGBA,GL_UNSIGNED_BYTE,setting?&on[0]:&off[0]);
   if(glGetError()!=GL_NO_ERROR) return 12;
  }
  if(scene==1) {
   if(on!=off) {fprintf(stderr,"FAIL ordinary gray changed with glow\n");return 13;}
   printf("PASS ordinary dark field byte-identical with glow\n");
   continue;
  }
  double directions[4]={0,0,0,0};
  int far_changes=0;
  for(int y=0;y<h;++y) for(int x=0;x<w;++x) {
   double dx=(x+0.5)*856/w-428.5,dy=(h-y-0.5)*400/h-200.5;
   double delta=linear(on[(y*w+x)*4])-linear(off[(y*w+x)*4]);
   if(fabs(dx)>3 || fabs(dy)>3) {if(fabs(delta)>0.00001) ++far_changes;}
   if(fabs(dy)<0.5 && dx>0.75) directions[0]+=delta;
   if(fabs(dy)<0.5 && dx<-0.75) directions[1]+=delta;
   if(fabs(dx)<0.5 && dy>0.75) directions[2]+=delta;
   if(fabs(dx)<0.5 && dy<-0.75) directions[3]+=delta;
  }
  printf("GLOW right/left/down/up extra linear sum %.6f %.6f %.6f %.6f; distant changed pixels=%d\n",directions[0],directions[1],directions[2],directions[3],far_changes);
  for(int i=0;i<4;++i) if(directions[i]<=0) return 14;
  if(far_changes) return 15;
  if(!save_pixels("build/crt-review/glow-off.bmp",w,h,off) || !save_pixels("build/crt-review/glow-25.bmp",w,h,on)) return 16;
 }
 return 0;
}

int main(int argc, char **argv)
{
 if (SDL_Init(SDL_INIT_VIDEO) != 0) return 1;
 SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
 SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
 SDL_Window *window = SDL_CreateWindow("CRT shader checks", 0, 0, 1440, 1080, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
 if (!window || !RA_CRTGL_Init(window)) return 2;
 setenv("RA_CRT_MODEL","beam",1);
 if(argc>1 && strcmp(argv[1],"--glow")==0) {
  int result=glow_checks(window,argc>2?argv[2]:0);
  RA_CRTGL_Shutdown();SDL_DestroyWindow(window);SDL_Quit();
  return result;
 }
 std::vector<uint32_t> source(640 * 400, 0xFF000000U);
 double dim_width = 0.0;
 int heights[] = {1080, 1440, 2160, 937};
 for (int size = 0; size < 4; ++size) {
  int h = heights[size], w = h * 4 / 3;
  SDL_SetWindowSize(window, w, h);
  RA_CRTGL_GetOutputSize(&w, &h);
  RAAspectViewport viewport = {0, 0, w, h};
  std::vector<unsigned char> pixels(w * h * 4);
  for (int strength = 0; strength <= 100; strength += 25) {
   for (int pattern = 0; pattern <= (size == 0 && strength == 0 ? 12 : strength == 25 ? 10 : 5); ++pattern) {
    if (pattern >= 11) {
     uint32_t value = pattern == 11 ? 0xFF404040U : 0xFFFFFFFFU;
     for (int x=0;x<640;++x) source[200*640+x] = value;
    }
    RACRTGLConfig config = {strength, 0, 0, 0, pattern >= 11 ? 0 : pattern};
    if (!RA_CRTGL_Present(&source[0], 640, 400, 640, viewport, config)) return 3;
    // Draw once more without swapping, using the active program/texture/uniforms.
    pglUseProgram(CRTProgram);
    glFinish(); // Drain the preceding Present before measuring this draw.
    Uint64 started = SDL_GetPerformanceCounter();
    glBegin(GL_QUADS);
    glTexCoord2f(0,0); glVertex2f(-1,1);
    glTexCoord2f(1,0); glVertex2f(1,1);
    glTexCoord2f(1,1); glVertex2f(1,-1);
    glTexCoord2f(0,1); glVertex2f(-1,-1);
    glEnd();
    glFinish();
    double milliseconds = 1000.0 * (SDL_GetPerformanceCounter() - started) / SDL_GetPerformanceFrequency();
    glReadBuffer(GL_BACK);
    glReadPixels(0,0,w,h,GL_RGBA,GL_UNSIGNED_BYTE,&pixels[0]);
    if (glGetError() != GL_NO_ERROR) return 4;
    if (pattern >= 11) {
     double sum = 0, first = 0, second = 0;
     for (int y=0;y<h;++y) {
      double v = linear(pixels[(y*w+w/2)*4]);
      sum += v; first += y*v; second += y*y*v;
     }
     double width = sqrt(second/sum - (first/sum)*(first/sum));
     if (pattern == 11) dim_width = width;
     else {
      printf("BEAM dim sigma=%.3f bright sigma=%.3f output pixels\n",dim_width,width);
      if (!(width > dim_width * 1.1)) return 7;
     }
     for (int x=0;x<640;++x) source[200*640+x] = 0xFF000000U;
    }
    if (pattern >= 6) {
     char path[160];
     sprintf(path,"build/crt-review/pattern-%d-%d.bmp",pattern,h);
     SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormatFrom(&pixels[0],w,h,32,w*4,SDL_PIXELFORMAT_RGBA32);
     if (!surface || SDL_SaveBMP(surface,path) != 0) return 6;
     SDL_FreeSurface(surface);
     printf("CAPTURE %s draw+finish=%.2fms\n",path,milliseconds);
     continue;
    }
    double mean[3] = {0,0,0};
    for (int i = 0; i < w*h; ++i) for (int c=0;c<3;++c) mean[c] += linear(pixels[i*4+c]) / (w*h);
    for (int c=0;c<3;++c) {
     double expected = pattern == 0 ? 0 : pattern == 2 ? linear(128) : pattern == 1 || pattern == c+3 ? 1 : 0;
     if (fabs(mean[c]-expected) > 0.006) {
      fprintf(stderr,"FAIL %dx%d strength=%d pattern=%d channel=%d mean=%.6f expected=%.6f\n",w,h,strength,pattern,c,mean[c],expected);
      return 5;
     }
    }
   }
  }
  printf("PASS %dx%d black/gray/white/primaries, strength 0/25/50/75/100\n",w,h);
 }
 RA_CRTGL_Shutdown(); SDL_DestroyWindow(window); SDL_Quit();
 return 0;
}
