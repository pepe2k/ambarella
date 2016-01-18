
#ifndef _MW_DC_IRIS_H_
#define _MW_DC_IRIS_H_

int enable_dc_iris(u8 enable);
int dc_iris_set_pid_coef(mw_dc_iris_pid_coef * pPid_coef);
int dc_iris_get_pid_coef(mw_dc_iris_pid_coef * pPid_coef);
#endif //  _MW_DC_IRIS_H_

