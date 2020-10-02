#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <tobii.h>
#include <tobii_streams.h>


/*VSでfopenを使うためのおまじない*/
#pragma warning(disable: 4996)

//ファイル作成用ポインタ
static FILE *fp_tobii_eye, *fp_tobii_eyep, *fp_tobii_headp;

//各データ格納用変数
int eyedata;
int eyepositiondata_left, eyepositiondata_right;
double wink_left, wink_right;
int headpositiondata;


//頭のポジション
void head_pose_callback(tobii_head_pose_t const* head_pose, void* user_data)
{
	if (head_pose->position_validity == TOBII_VALIDITY_VALID) {
		fprintf(fp_tobii_headp, "Position:,%f,%f,%f,",
			head_pose->position_xyz[0],
			head_pose->position_xyz[1],
			head_pose->position_xyz[2]);

		if (headpositiondata == 0) {
			headpositiondata = 1;
		}
	}
	else {
		fprintf(fp_tobii_headp, "Position:,-1,-1,-1,");

		if (headpositiondata == 1) {
			headpositiondata = 0;
		}
	}

	if (head_pose->position_validity == TOBII_VALIDITY_VALID) {
		fprintf(fp_tobii_headp, "Rotation:,%f,%f,%f\n",
			head_pose->rotation_xyz[0],
			head_pose->rotation_xyz[1],
			head_pose->rotation_xyz[2]);
	}
	else {
		fprintf(fp_tobii_headp, "Rotation:,-1,-1,-1\n");
	}

}


//目の位置
void eye_position_callback(tobii_eye_position_normalized_t const* eye_pos, void* user_data)
{
	if (eye_pos->left_validity == TOBII_VALIDITY_VALID) {
		fprintf(fp_tobii_eyep, "Left:,%f,%f,%f,",
			eye_pos->left_xyz[0],
			eye_pos->left_xyz[1],
			eye_pos->left_xyz[2]);
	}
	else {
		fprintf(fp_tobii_eyep, "Left:,-1,-1,-1,");
		if (eyepositiondata_left == 1) {
			eyepositiondata_left = 0;
		}
	}

	if (eye_pos->right_validity == TOBII_VALIDITY_VALID) {

		fprintf(fp_tobii_eyep, "Right:,%f,%f,%f\n",
			eye_pos->right_xyz[0],
			eye_pos->right_xyz[1],
			eye_pos->right_xyz[2]);

	}
	else {
		fprintf(fp_tobii_eyep, "Right:,-1,-1,-1\n");
		if (eyepositiondata_right == 1) {
			eyepositiondata_right = 0;
		}
	}


}


//視線計測
static void gaze_point_callback(tobii_gaze_point_t const* gaze_point, void* user_data)
{
	double x, y;
	if (gaze_point->validity == TOBII_VALIDITY_VALID) {
		fprintf(fp_tobii_eye, "%f,%f\n",
			gaze_point->position_xy[0],
			gaze_point->position_xy[1]);

		x = double(gaze_point->position_xy[0]);
		y = double(gaze_point->position_xy[1]);


	}
	else {
		fprintf(fp_tobii_eye, "-1,-1\n");
	}

}


static void url_receiver(char const* url, void* user_data)
{
	char* buffer = (char*)user_data;
	if (*buffer != '\0') return; // only keep first value

	if (strlen(url) < 256)
		strcpy(buffer, url);
}


int tobii()
{
	/*ファイル用*/
	char filename[3][50];

	//ファイルを作成---------------------------------------------------
	sprintf(filename[0], "eye.csv");
	sprintf(filename[1], "eyep.csv");
	sprintf(filename[2], "headp.csv");
	fp_tobii_eye = fopen(filename[0], "a");
	fp_tobii_eyep = fopen(filename[1], "a");
	fp_tobii_headp = fopen(filename[2], "a");
	//-----------------------------------------------------------------

	eyedata = 0;
	eyepositiondata_left = 0;
	eyepositiondata_right = 0;
	wink_left = 0;
	wink_right = 0;
	headpositiondata = 0;

	tobii_api_t* api;
	tobii_error_t error = tobii_api_create(&api, NULL, NULL);
	assert(error == TOBII_ERROR_NO_ERROR);

	char url[256] = { 0 };
	error = tobii_enumerate_local_device_urls(api, url_receiver, url);
	assert(error == TOBII_ERROR_NO_ERROR && *url != '\0');

	tobii_device_t* device;
	error = tobii_device_create(api, url, TOBII_FIELD_OF_USE_INTERACTIVE, &device);
	assert(error == TOBII_ERROR_NO_ERROR);

	//頭の位置計測を有効化
	error = tobii_head_pose_subscribe(device, head_pose_callback, 0);
	assert(error == TOBII_ERROR_NO_ERROR);

	//目の位置計測を有効化
	error = tobii_eye_position_normalized_subscribe(device, eye_position_callback, 0);
	assert(error == TOBII_ERROR_NO_ERROR);

	//目線の計測を有効化
	error = tobii_gaze_point_subscribe(device, gaze_point_callback, 0);
	assert(error == TOBII_ERROR_NO_ERROR);


	//とりあえず100データぶん
	for (int i = 0; i <= 100; i++) {

		//計測を呼びだし（有効化したもの全部，結構重くなる）
		error = tobii_wait_for_callbacks(1, &device);
		assert(error == TOBII_ERROR_NO_ERROR || error == TOBII_ERROR_TIMED_OUT);

		error = tobii_device_process_callbacks(device);
		assert(error == TOBII_ERROR_NO_ERROR);


	}
	
	//デバイスの初期化
	error = tobii_gaze_point_unsubscribe(device);
	assert(error == TOBII_ERROR_NO_ERROR);

	error = tobii_device_destroy(device);
	assert(error == TOBII_ERROR_NO_ERROR);

	error = tobii_api_destroy(api);
	assert(error == TOBII_ERROR_NO_ERROR);

	//エクセルデータをfclose処理（fcloseをしないとうまく書き出されないので注意）
	fclose(fp_tobii_eye);
	fclose(fp_tobii_eyep);
	fclose(fp_tobii_headp);

	return 1;

}
