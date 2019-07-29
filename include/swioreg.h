/* by Shuhei Ajimura */

#ifdef __cplusplus
extern "C" {
#endif

int swio_open(void);
void swio_close(void);
int swio_w(unsigned int /*addr*/, unsigned int /*data*/);
int swio_r(unsigned int /*addr*/, unsigned int */*data*/);
int swio_reset();
int swio_read(unsigned char */*tag*/);
int swio_done();
int swio_show();

#ifdef __cplusplus
};
#endif
