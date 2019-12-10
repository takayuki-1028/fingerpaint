
#include <QtGui>
#include <QPlainTextEdit>
#include <QPainter>
#include <QPen>
#include <QPointF>

#include "scribblearea.h"


#include "geometricUtil.h"
#include "timeUtil.h"




ScribbleArea::ScribbleArea(QWidget *parent)
    : QWidget(parent)
{
	GetSystemTimeAsFileTime(&launchTime);					//システムタイムをファイルタイムとして取得する

    setAttribute(Qt::WA_AcceptTouchEvents);					//WA_AcceptTouchEvents属性の設定
    setAttribute(Qt::WA_StaticContents);					//WA_StaticContents属性の設定
    modified = false;										//ScribbleAreaに変更がない状態にする

    myPenColors												//Qcolorで指定した色をリスト化して保存
            << QColor("green") << QColor("purple") << QColor("red") << QColor("blue")<< QColor("yellow")
            << QColor("pink") << QColor("orange") << QColor("brown")<< QColor("grey") << QColor("black");

	this->resetPoints();									//初期化

	qApp->installEventFilter(this);							//イベントのインストール
}


bool
ScribbleArea::openVolume(const QString &fileName)			//ボリュームデータの読み込み
{
    return true;
}

bool
ScribbleArea::saveImage(const QString &fileName, const char *fileFormat)	//ファイル名とフォーマットを指定？？？
{
    QImage visibleImage = *qtImage;

    resizeImage(&visibleImage, size());										//resizeImage呼び出し:サイズ変更

    if (visibleImage.save(fileName, fileFormat))							//visibleImage.saveのファイル名とフォーマットに変更がない場合
	{
        modified = false;													//変更なしの状態にする
        return true;
    }
	else
	{
        return false;
    }
}

void
ScribbleArea::resetView()		//File→Reset View
{
	initializeViewParams();
	setResiliceImage();
	update();
}


void ScribbleArea::print() {}	//File→Print 


void ScribbleArea::displayModeString(QPainter* painter)													//モードの決定
{
	int	string_start_x = 1450;
	int	string_start_y = 30;

	char	modeString[128];

	painter->setPen(QColor(255,255,255,192));			//painter->setPen(QColor("red"));
	painter->setFont(QFont("Times", 18, QFont::Bold));	// Helvetica, Times, Fantasy, Courier

	sprintf(modeString, " ");

	switch (this->fingerTouchMode)
	{
		//case MODE_NO_FINGER:	sprintf(modeString, "NO FINGER");							break;
		case MODE_1_FINGER:		sprintf(modeString, "2D drag (1 FINGER)");					break;			//1本モード	  画像の平行移動
		case MODE_2_FINGERS:	sprintf(modeString, "Scale and 2D rotation (2 FINGERS)");	break;			//２本モード　画像の回転、拡大縮小
		case MODE_3_FINGERS:	sprintf(modeString, "Slice paging (3 FINGERS)");			break;			//３本モード　画像の奥行
		case MODE_4_FINGERS:	sprintf(modeString, "3D rotation (4 FINGERS)");				break;			//４本モード　画像の軸回転
		case MODE_5_FINGERS:	sprintf(modeString, "Grayscaling (5 FINGERS)");				break;			//５本モード　画像の明暗
		case MODE_6_FINGERS:	sprintf(modeString, "Spot MIP (6 FINGERS)");				break;			//６本モード
		case MODE_10_FINGERS:	sprintf(modeString, "Reset view (10 FINGERS)");				break;			//１０本モード　リセット

																											//それぞれのモードの待機状態（指を動かす前でモード変更可能状態）
		case MODE_TMP_1_FINGER:		sprintf(modeString, "2D drag (1 FINGER)");					break;		
		case MODE_TMP_2_FINGERS:	sprintf(modeString, "Scale and 2D rotation (2 FINGERS)");	break;
		case MODE_TMP_3_FINGERS:	sprintf(modeString, "Slice paging (3 FINGERS)");			break;
		case MODE_TMP_4_FINGERS:	sprintf(modeString, "3D rotation (4 FINGERS)");				break;
		case MODE_TMP_5_FINGERS:	sprintf(modeString, "Grayscaling (5 FINGERS)");				break;
		case MODE_TMP_6_FINGERS:	sprintf(modeString, "Spot MIP (6 FINGERS)");				break;
		case MODE_TMP_10_FINGERS:	sprintf(modeString, "Reset view (10 FINGERS)");				break;

		//case MODE_TMP_1_FINGER:		sprintf(modeString, "TMP 1 FINGER");		break;
		//case MODE_TMP_2_FINGERS:	sprintf(modeString, "TMP 2 FINGERS");		break;
		//case MODE_TMP_3_FINGERS:	sprintf(modeString, "TMP 3 FINGERS");		break;
		//case MODE_TMP_4_FINGERS:	sprintf(modeString, "TMP 4 FINGERS");		break;
		//case MODE_TMP_5_FINGERS:	sprintf(modeString, "TMP 5 FINGERS");		break;
		//case MODE_TMP_6_FINGERS:	sprintf(modeString, "TMP 6 FINGERS");		break;
		//case MODE_TMP_10_FINGERS:	sprintf(modeString, "TMP 10 FINGERS");		break;

		//case MODE_INT_3_FINGERS:	sprintf(modeString, "Slice paging (FIXED)");	break;
		case MODE_INT_4_FINGERS:	sprintf(modeString, "3D rotation (FIXED)");		break;					//4本モード時の軸を決めるモード
		//case MODE_INT_5_FINGERS:	sprintf(modeString, "Grayscaling (FIXED)");		break;

		default:
			break;
	}

	painter->drawText(QPoint(string_start_x, string_start_y), modeString);									//現在のモードを表示
}

void
ScribbleArea::paintEvent(QPaintEvent *event)
{
	QPainter painter(this);
	const QRect rect = event->rect();

	if (volume == NULL || volImage == NULL)	return;															//volumeかvolImageがNULLのとき関数から抜ける

	//////////////////////////////////////////////////////////this->resetTimer();
	
	if( qtImage!=NULL)	painter.drawImage(rect.topLeft(), *qtImage, rect);									//qtImageがNULLじゃない時　qtImageの画像を表示？

	drawTime += getTimeInMillisec();	drawCount++;														//触ってからの時間を計測
	//this->showTime("image draw");

	int		iDiameter = BUTTON_DIAMETER;																	//120
	qreal	qrDiameter = qreal(iDiameter);
	int		shift_y_for_fix = 50;

	int	qfmb_pos_x = iDiameter / 3;																			// qfmb: quit fixed mode button
	int	qfmb_pos_y = iDiameter / 3;
	int qfmb_iDiameter = iDiameter / 2;
	qreal	qfmb_qrD = qreal(qfmb_iDiameter);


	
	if (rect_s.isEmpty())	rect_s.setSize(QSizeF(qrDiameter, qrDiameter));									// rect_sが空の時　INT. 4F MODE: ROT AXIS START POINT
	if (rect_e.isEmpty())	rect_e.setSize(QSizeF(qrDiameter, qrDiameter));									// rect_eが空の時  INT. 4F MODE: ROT AXIS END POINT
	if (rect_b.isEmpty())	rect_b.setSize(QSizeF(qfmb_qrD, qfmb_qrD));										// rect_bが空の時  INT. nF MODE QUIT BUTTON

	if (INTmode3_shift_button.isEmpty())	INTmode3_shift_button.setSize(QSizeF(qrDiameter, qrDiameter));	//INTmode3_shift_buttonが空の時
	if (INTmode4_shift_button.isEmpty())	INTmode4_shift_button.setSize(QSizeF(qrDiameter, qrDiameter));	//INTmode4_shift_buttonが空の時
	if (INTmode5_shift_button.isEmpty())	INTmode5_shift_button.setSize(QSizeF(qrDiameter, qrDiameter));	//INTmode5_shift_buttonが空の時
	if (INTmode6_shift_button.isEmpty())	INTmode6_shift_button.setSize(QSizeF(qrDiameter, qrDiameter));	//INTmode5_shift_buttonが空の時

	

	int	penWidth_button = 8;
	int	penWidth_line = 4;

	switch (this->fingerTouchMode)
	{
#ifdef STOP
		case MODE_INT_3_FINGERS:	

			painter.setPen(QPen(Qt::yellow, penWidth_button));
			INTmode3_shift_button.moveCenter(QPoint(top_pos.x, (top_pos.y - iDiameter)));
			painter.drawEllipse(rect_b);
			break;
#endif
		case MODE_TMP_4_FINGERS:

			painter.setPen(QPen(Qt::white, penWidth_button));	// INT. 4F MODE: FIX BUTTON
			INTmode4_shift_button.moveCenter(QPoint((upper_right_pos.x + iDiameter), (upper_right_pos.y - iDiameter + shift_y_for_fix)));
			painter.drawEllipse(INTmode4_shift_button);
			break;

		case MODE_INT_4_FINGERS:	

			if (line_start_x != 0 && line_start_y != 0 && line_end_x != 0 && line_end_y != 0)
			{
				// axis line
				{
					VOL_VECTOR2D	vec;

					vec.x = line_end_x - line_start_x;
					vec.y = line_end_y - line_start_y;

					VOL_NormalizeVector2D(&vec);

					painter.setPen(QPen(QColor(0, 255, 255, 128), penWidth_line / 2));	// painter.setPen(QPen(Qt::red, penWidth_line / 2));
					painter.drawLine(	line_start_x + (int)(vec.x * qrDiameter/2),
										line_start_y + (int)(vec.y * qrDiameter/2),
										line_end_x - (int)(vec.x * qrDiameter/2),
										line_end_y - (int)(vec.y * qrDiameter/2) );
				}

				// axis start/end circles 
				{
					/*
					painter.setPen(QPen(QColor(0, 255, 255, 128), penWidth_line / 2));
					if (rect_s.isEmpty())	rect_s.setSize(QSizeF(qrDiameter*0.8, qrDiameter*0.8));		// INT. 4F MODE: ROT AXIS START POINT
					if (rect_e.isEmpty())	rect_e.setSize(QSizeF(qrDiameter*0.8, qrDiameter*0.8));		// INT. 4F MODE: ROT AXIS END POINT
					rect_s.moveCenter(QPoint(line_start_x, line_start_y));
					rect_e.moveCenter(QPoint(line_end_x, line_end_y));

					painter.drawEllipse(rect_s);
					painter.drawEllipse(rect_e);
					*/

					painter.setPen(QPen(QColor(0, 255, 255, 128), penWidth_line));	// painter.setPen(QPen(Qt::green, penWidth_button));
					if (rect_s.isEmpty())	rect_s.setSize(QSizeF(qrDiameter, qrDiameter));		// INT. 4F MODE: ROT AXIS START POINT
					if (rect_e.isEmpty())	rect_e.setSize(QSizeF(qrDiameter, qrDiameter));		// INT. 4F MODE: ROT AXIS END POINT
					rect_s.moveCenter(QPoint(line_start_x, line_start_y));
					rect_e.moveCenter(QPoint(line_end_x, line_end_y));

					painter.drawEllipse(rect_s);
					painter.drawEllipse(rect_e);
				}

				// quit button (X)
				{
					painter.setPen(QPen(QColor(255, 255, 255, 255), penWidth_button));	// painter.setPen(QPen(Qt::white, penWidth_button));
					
					rect_b.moveCenter(QPoint(qfmb_pos_x, qfmb_pos_y));			//rect_b.moveCenter(QPoint(1790, 890));
					painter.drawEllipse(rect_b);

					int		shift_for_X = (int)((float)qfmb_iDiameter / sqrtf(2.0f)) / 2;

					painter.drawLine(	qfmb_pos_x - shift_for_X, qfmb_pos_y - shift_for_X,
										qfmb_pos_x + shift_for_X, qfmb_pos_y + shift_for_X);
					painter.drawLine(	qfmb_pos_x - shift_for_X, qfmb_pos_y + shift_for_X,
										qfmb_pos_x + shift_for_X, qfmb_pos_y - shift_for_X);
				}
			}
			break;

#ifdef STOP
		case MODE_TMP_5_FINGERS:

			painter.setPen(QPen(Qt::white, penWidth_button));
			INTmode5_shift_button.moveCenter(QPoint(top_pos.x, (top_pos.y - iDiameter)));
			painter.drawEllipse(INTmode5_shift_button);
			break;

		case MODE_INT_5_FINGERS:
		
			painter.setPen(QColor("white"));
			painter.setPen(QPen(Qt::yellow, penWidth_button));
			painter.drawEllipse(rect_b);
			break;
#endif
		case MODE_TMP_6_FINGERS:

			painter.setPen(QPen(Qt::yellow, penWidth_button));	// INT. 6F MODE: FIX BUTTON
			INTmode6_shift_button.moveCenter(QPoint((top_pos.x), (top_pos.y)));
			painter.drawEllipse(INTmode6_shift_button);
			break;

		default:
			break;
	}


	// string
	displayModeString(&painter);

}

void
ScribbleArea::resizeEvent(QResizeEvent *event)
{
	this->updateSliceImageSize();
	this->setResiliceImage();

	update();

	QWidget::resizeEvent(event);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void
ScribbleArea::resizeImage(QImage *image, const QSize &newSize)
{
    if (image->size() == newSize)	return;						//サイズが同じ時関数を抜ける。

    QImage newImage(newSize, QImage::Format_RGB32);
    newImage.fill(qRgb(255, 255, 255));
    QPainter painter(&newImage);
    painter.drawImage(QPoint(0, 0), *image);
    *image = newImage;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void
convert_to_UINT8_image(VOL_RAWIMAGEDATA* ssImage, VOL_RAWIMAGEDATA* ucImage, VOL_VALUERANGE* greyscale )
{
	short*	ssPixels1D = (short*)ssImage->data[0];
	unsigned char* ucPixels1D = (unsigned char*)ucImage->data[0];

	float min = greyscale->min, max = greyscale->max, range = max - min;
	int width = ssImage->matrixSize->width, height = ssImage->matrixSize->height;

	for (int j = 0; j<ssImage->matrixSize->height; j++)
	for (int i = 0; i<ssImage->matrixSize->width; i++)
	{
		float			pixVal = *ssPixels1D++;
		unsigned char	index;

		if (pixVal < min)	index = 0;
		else
			if (pixVal > max)	index = 255;
			else
				index = (unsigned char)(255.0f * (pixVal - min) / range);

		*ucPixels1D++ = index;
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void
ScribbleArea::setResiliceImage()
{
	if (this->volImage == NULL || this->volImageLQ == NULL ||
		this->volImageUI8 == NULL || this->volImageUI8LQ == NULL)	return;

	/////////////////////////////this->resetTimer();
	
	if (resliceLQ == false)
	{
		//VOL_VECTOR3D	u, v;

		memcpy(&u, &eu, sizeof(VOL_VECTOR3D));	VOL_ScaleVector3D(&u, psize);
		memcpy(&v, &ev, sizeof(VOL_VECTOR3D));	VOL_ScaleVector3D(&v, psize);

		VOL_GetObliqueSection(volume, 0, &Po, &u, &v, volImage, 0);

		HQresliceTime += getTimeInMillisec();	HQresliceCount++;
		//this->showTime("HQ reslice");
		/////////////////////////////this->resetTimer();

		convert_to_UINT8_image(volImage, volImageUI8, &greyscale);

		HQconvertTime += getTimeInMillisec();	HQconvertCount++;
		//this->showTime("HQ pixel unit convert");
	}
	else
	{
		//VOL_VECTOR3D	u, v;
		VOL_INTSIZE2D	origSize;

		origSize.width = volImage->matrixSize->width;
		origSize.height = volImage->matrixSize->height;

		memcpy(&u, &eu, sizeof(VOL_VECTOR3D));	VOL_ScaleVector3D(&u, psize*LQ_FACTOR);
		memcpy(&v, &ev, sizeof(VOL_VECTOR3D));	VOL_ScaleVector3D(&v, psize*LQ_FACTOR);

		VOL_GetObliqueSection(volume, 0, &Po, &u, &v, volImageLQ, 0);

		LQresliceTime += getTimeInMillisec();	LQresliceCount++;
		//this->showTime("LQ reslice");
		/////////////////////////////this->resetTimer();

		convert_to_UINT8_image(volImageLQ, volImageUI8LQ, &greyscale);

		VOL_DeleteRawImageData(volImageUI8);

		volImageUI8 = VOL_DuplicateRawImageData(volImageUI8LQ);

		VOL_ScaleRawImageData(volImageUI8, &origSize, VOL_SCALING_METHOD_NEAREST_NEIGHBOUR);	// VOL_SCALING_METHOD_NEAREST_NEIGHBOUR	/ VOL_SCALING_METHOD_LINEAR
	
		LQconvertTime += getTimeInMillisec();	LQconvertCount++;
		//this->showTime("LQ pixel unit convert");
	}

	//this->resetTimer();

	if(qtImage!=NULL)	delete qtImage;
	qtImage = new QImage( (unsigned char*)volImageUI8->data[0], volImageUI8->matrixSize->width, volImageUI8->matrixSize->height, QImage::Format_Indexed8);
	if (qtImage != NULL)	qtImage->setColorTable(this->colorTable);

	//this->showTime("convert to QImage");
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void
ScribbleArea::updateSliceImageSize()
{
	if (this->volImage != NULL)			VOL_DeleteRawImageData(this->volImage);
	if (this->volImageLQ != NULL)		VOL_DeleteRawImageData(this->volImageLQ);
	if (this->volImageUI8 != NULL)		VOL_DeleteRawImageData(this->volImageUI8);
	if (this->volImageUI8LQ != NULL)	VOL_DeleteRawImageData(this->volImageUI8LQ); 
	

	VOL_INTSIZE2D	matSize, matSizeLQ;

	// CAUTION !! needs alignment (padding)
	int	align_size = 8;
	matSize.width = (width() / align_size + 1) * align_size;
	matSize.height = (height() / align_size + 1) * align_size;
	matSizeLQ.width = matSize.width / LQ_FACTOR;
	matSizeLQ.height = matSize.height / LQ_FACTOR;

	this->volImage = VOL_NewSingleChannelRawImageData(&matSize, this->volume->voxelUnit[0], this->volume->voxelType[0]);
	this->volImageLQ = VOL_NewSingleChannelRawImageData(&matSizeLQ, this->volume->voxelUnit[0], this->volume->voxelType[0]);
	this->volImageUI8 = VOL_NewSingleChannelRawImageData(&matSize, VOL_VALUEUNIT_UINT8, VOL_VALUETYPE_SINGLE);
	this->volImageUI8LQ = VOL_NewSingleChannelRawImageData(&matSizeLQ, VOL_VALUEUNIT_UINT8, VOL_VALUETYPE_SINGLE);


	if (this->volImage == NULL || this->volImageUI8 == NULL)	return;

	if (qtImage != NULL)	delete qtImage;
	qtImage = new QImage((unsigned char*)volImageUI8->data[0], volImageUI8->matrixSize->width, volImageUI8->matrixSize->height, QImage::Format_Indexed8);
	if (qtImage != NULL)	qtImage->setColorTable(this->colorTable);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void
ScribbleArea::setVolumeData(VOL_RAWVOLUMEDATA* v, VOL_VALUERANGE* gs)
{
	if (v == NULL)
	{
		printf("volume==NULL?\n");
		return;
	}

	this->volume = v;

	// clear border
	{
		VOL_RAWVOLUMEDATA* tmp = VOL_ExtractSingleChannelRawVolumeData(v, 0);

		VOL_ClearRawVolumeData(v, 0);

		VOL_INTVECTOR3D theOrigin;
		theOrigin.x = theOrigin.y = theOrigin.z = 1;

		VOL_INTSIZE3D theSize;
		theSize.width = v->matrixSize->width - 2;
		theSize.height = v->matrixSize->height - 2;
		theSize.depth = v->matrixSize->depth - 2;

		VOL_INTBOX3D* theBox = VOL_NewIntBox3D(&theOrigin, &theSize);

		VOL_CopyRawVolumeData(tmp, 0, v, 0, theBox, &theOrigin);
		VOL_DeleteIntBox3D(theBox);
	}

	memcpy(&this->init_greyscale, gs, sizeof(VOL_VALUERANGE));

	for (int i = 0; i < 256; i++)	colorTable.push_back(QColor(i, i, i).rgb());

	this->initializeViewParams();
	this->updateSliceImageSize();
	this->setResiliceImage();
}

 
void
ScribbleArea::initializeViewParams()
{
	if (this->volume == NULL)	return;

	eu.x = 1.0f;	eu.y = 0.0f;	eu.z = 0.0f;
	ev.x = 0.0f;	ev.y = 0.0f;	ev.z = 1.0f;
	VOL_OuterProduct3D(&eu, &ev, &ew);
	
	psize = qMax( (float)volume->matrixSize->width/width(), (float)volume->matrixSize->depth/height() );

	printf("current window size: %d x %d\n", width(), height());

	Po.x = volume->matrixSize->width / 2 - psize*( width() / 2 * eu.x + height() / 2 * ev.x);
	Po.y = volume->matrixSize->height / 2 - psize * ( width() / 2 * eu.y + height() / 2 * ev.y);
	Po.z = volume->matrixSize->depth / 2 - psize * ( width() / 2 * eu.z + height() / 2 * ev.z);

	memcpy(&this->greyscale, &this->init_greyscale, sizeof(VOL_VALUERANGE));
}

void
ScribbleArea::memoryPreviousViewParams()
{
	VOL_OuterProduct3D(&eu, &ev, &ew);

	memcpy(&prev_Po, &Po, sizeof(VOL_VECTOR3D));
	memcpy(&prev_eu, &eu, sizeof(VOL_VECTOR3D));
	memcpy(&prev_ev, &ev, sizeof(VOL_VECTOR3D));
	memcpy(&prev_ew, &ew, sizeof(VOL_VECTOR3D));
	memcpy(&prev_psize, &psize, sizeof(float));
	memcpy(&prev_greyscale, &greyscale, sizeof(VOL_VALUERANGE));
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void
ScribbleArea::getSliceRotationGeometry(VOL_VECTOR2D* pair1midPnt, VOL_VECTOR2D* pair2midPnt, float* pair1rotAngle, float* pair2rotAngle)
{
	// sort by distances to the first touch point
	sort4points();

	// 2 mid-points fore rotation axis
	get_midpoint_2D( &(startPos[ sortedIndices[0] ]), &(startPos[ sortedIndices[1] ]), pair1midPnt );
	get_midpoint_2D( &(startPos[ sortedIndices[2] ]), &(startPos[ sortedIndices[3] ]), pair2midPnt );

	VOL_VECTOR2D start1to0, current1to0, start3to2, current3to2;

	subtract_vector_2D( &(startPos[sortedIndices[0]]), &(startPos[sortedIndices[1]]), &start1to0 );
	subtract_vector_2D( &(currentPos[sortedIndices[0]]), &(currentPos[sortedIndices[1]]), &current1to0 );

	subtract_vector_2D( &(startPos[sortedIndices[2]]), &(startPos[sortedIndices[3]]), &start3to2);
	subtract_vector_2D( &(currentPos[sortedIndices[2]]), &(currentPos[sortedIndices[3]]), &current3to2);

	float angleChangePair1, angleChangePair2;
	float lengthChange;	// unused
	
	get_changes_of_vector_angle_and_length_2D( &start1to0, &current1to0, pair1rotAngle, &lengthChange );
	get_changes_of_vector_angle_and_length_2D( &start3to2, &current3to2, pair2rotAngle, &lengthChange );
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void
ScribbleArea::slideSlice(float move_u, float move_v)
{
	Po.x = prev_Po.x - (move_u * psize * prev_eu.x + move_v * psize * prev_ev.x);
	Po.y = prev_Po.y - (move_u * psize * prev_eu.y + move_v * psize * prev_ev.y);
	Po.z = prev_Po.z - (move_u * psize * prev_eu.z + move_v * psize * prev_ev.z);
}


void
ScribbleArea::pushSlice(float distance)
{
	Po.x = prev_Po.x + distance * prev_ew.x;
	Po.y = prev_Po.y + distance * prev_ew.y;
	Po.z = prev_Po.z + distance * prev_ew.z;
}

void
ScribbleArea::pinchSlice(float theta, float scale, VOL_VECTOR2D* start0to1, VOL_VECTOR2D* current0to1)
{
	VOL_VECTOR3D P1;	// must be unchanged in 3D

	get_3D_position_by_2D_position( &P1, start0to1, &prev_Po, &prev_eu, &prev_ev, prev_psize );

	float cos_theta = cosf(theta), sin_theta = sinf(theta);

	// rotate eu and ev in -theta
	eu.x = cos_theta * prev_eu.x - sin_theta * prev_ev.x;
	eu.y = cos_theta * prev_eu.y - sin_theta * prev_ev.y;
	eu.z = cos_theta * prev_eu.z - sin_theta * prev_ev.z;
	ev.x = sin_theta * prev_eu.x + cos_theta * prev_ev.x;
	ev.y = sin_theta * prev_eu.y + cos_theta * prev_ev.y;
	ev.z = sin_theta * prev_eu.z + cos_theta * prev_ev.z;

	VOL_NormalizeVector3D(&eu);
	VOL_NormalizeVector3D(&ev);

	psize = prev_psize / scale;

	//Po.x = P1.x - psize * (current0to1->x * eu.x + current0to1->y * ev.x);
	//Po.y = P1.y - psize * (current0to1->x * eu.y + current0to1->y * ev.y);
	//Po.z = P1.z - psize * (current0to1->x * eu.z + current0to1->y * ev.z);

	get_3D_position_by_2D_position( &Po, current0to1, &P1, &eu, &ev, -psize );
}

void
ScribbleArea::rotateSlice(VOL_VECTOR2D* pair1midPnt, VOL_VECTOR2D* pair2midPnt, float pair1RightScrewAngle)
{
	VOL_VECTOR3D mp1, mp2;
	VOL_VECTOR3D rotAxisDir, rotAxisOrigin;
	VOL_VECTOR3D HofPo, rotU, rotV, rotW;
	
	get_3D_position_by_2D_position( &mp1, pair1midPnt, &prev_Po, &prev_eu, &prev_ev, prev_psize );
	get_3D_position_by_2D_position( &mp2, pair2midPnt, &prev_Po, &prev_eu, &prev_ev, prev_psize );

	memcpy( &rotAxisOrigin, &mp1, sizeof(VOL_VECTOR3D) );
	subtract_vector_3D( &mp2, &mp1, &rotAxisDir );
	VOL_NormalizeVector3D(&rotAxisDir);

	memcpy( &rotW, &rotAxisDir, sizeof(VOL_VECTOR3D) );

	get_foot_of_perpendicular_3D( &prev_Po, &rotAxisDir, &rotAxisOrigin, &HofPo);

	subtract_vector_3D( &prev_Po, &HofPo, &rotU );
	VOL_NormalizeVector3D(&rotU);

	VOL_OuterProduct3D( &rotW, &rotU, &rotV);

	memcpy( &Po, &prev_Po, sizeof(VOL_VECTOR3D) );
	rotate_point_3D( &Po, &rotAxisOrigin, &rotU, &rotV, &rotW, pair1RightScrewAngle);

	VOL_VECTOR3D Po_plus_eu, Po_plus_ev;

	add_vector_3D( &prev_Po, &prev_eu, &Po_plus_eu );
	rotate_point_3D( &Po_plus_eu, &rotAxisOrigin, &rotU, &rotV, &rotW, pair1RightScrewAngle );
	subtract_vector_3D( &Po_plus_eu, &Po, &eu );
	VOL_NormalizeVector3D(&eu);

	add_vector_3D(&prev_Po, &prev_ev, &Po_plus_ev);
	rotate_point_3D(&Po_plus_ev, &rotAxisOrigin, &rotU, &rotV, &rotW, pair1RightScrewAngle);
	subtract_vector_3D(&Po_plus_ev, &Po, &ev);
	VOL_NormalizeVector3D(&ev);
}



void
ScribbleArea::changeGreyscale(float move_u, float move_v)
{
	float range = prev_greyscale.max - prev_greyscale.min;
	float range_center = (prev_greyscale.min + prev_greyscale.max) / 2.0f;

	range_center += move_v;
	range -= move_u;

	if (range < 1.0f)	range = 1.0f;

	greyscale.max = range_center + range / 2.0f;
	greyscale.min = range_center - range / 2.0f;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


bool
ScribbleArea::event(QEvent *event)
{
	switch (event->type())
	{
		case QEvent::TouchBegin:
		case QEvent::TouchUpdate:
		case QEvent::TouchEnd:

			processTouchEvent(event);
			break;

		case QEvent::MouseButtonPress:
		case QEvent::MouseMove:
		case QEvent::MouseButtonRelease:
		case QEvent::MouseButtonDblClick:

			//processMouseEvent( event );
			break;

		default:
			return QWidget::event(event);
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void 
ScribbleArea::setMouseEventPos(QMouseEvent* mouseEvent, VOL_INTVECTOR2D * pos)
{
	pos->x = mouseEvent->x();
	pos->y = mouseEvent->y();
}


bool
ScribbleArea::processMouseEvent(QEvent *event)
{
	switch (event->type())
	{
		case QEvent::MouseButtonPress:
		{
			setMouseEventPos(static_cast<QMouseEvent *>(event), &this->mouse_press_pos);
			mouse_press_button = (static_cast<QMouseEvent *>(event))->buttons();
			memoryPreviousViewParams();
		}
		break;

		case QEvent::MouseMove:
		{
			setMouseEventPos(static_cast<QMouseEvent *>(event), &this->mouse_move_pos);

			switch (mouse_press_button)
			{
			case Qt::LeftButton:
				slideSlice((float)(mouse_move_pos.x - mouse_press_pos.x),(float)(mouse_move_pos.y - mouse_press_pos.y));
				break;
			case Qt::MidButton:
				pushSlice((float)(mouse_move_pos.y - mouse_press_pos.y)*psize);
				break;
			case Qt::RightButton:
				changeGreyscale((float)(mouse_move_pos.x - mouse_press_pos.x), (float)(mouse_move_pos.y - mouse_press_pos.y));
				break;
			default:
				break;
			}
			setResiliceImage();
			update();
			break;
		}
		case QEvent::MouseButtonDblClick:
		{
			initializeViewParams();
			setResiliceImage();
			update();
			break;
		}
		case QEvent::MouseButtonRelease:
		default:
			break;
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void
ScribbleArea::getFingerTouchNum()
{
		fingerTouchNum = 0;

	for (int i = 0; i < MAX_TOUCH_NUM; i++)	fingerTouchNum += finger_touch_or_not[i];
}

void
ScribbleArea::updateFingerTouchMode(const QTouchEvent::TouchPoint touchPoint)
{
	if (fingerTouchNum == 0 || fingerTouchNum > 10)	return;

	if (fingerTouchMode == MODE_NO_FINGER)	// NONE mode
	{
		fingerTouchMode = -fingerTouchNum;
		return;
	}
	else
		if (fingerTouchMode < 0)	// TMP modes
		{
			if (max_displacement_among_fingers > MAX_NEGLISIBLE_DISPLACEMENT)
			{
				fingerTouchMode *= -1;	// confirmed
				return;
			}

			///////////
			if (fingerTouchMode == MODE_TMP_3_FINGERS || fingerTouchMode == MODE_TMP_4_FINGERS || fingerTouchMode == MODE_TMP_5_FINGERS || fingerTouchMode == MODE_TMP_6_FINGERS)
			{
				int range = BUTTON_DIAMETER/2;

				if (startPos[3].x - range <= INTmode3_shift_button.center().x() && INTmode3_shift_button.center().x() <= startPos[3].x + range &&
					startPos[3].y - range <= INTmode3_shift_button.center().y() && INTmode3_shift_button.center().y() <= startPos[3].y + range)
				{
					return;
				}

				if (startPos[4].x - range <= INTmode4_shift_button.center().x() && INTmode4_shift_button.center().x() <= startPos[4].x + range &&
					startPos[4].y - range <= INTmode4_shift_button.center().y() && INTmode4_shift_button.center().y() <= startPos[4].y + range)
				{
					return;
				}

				if (startPos[5].x - range <= INTmode5_shift_button.center().x() && INTmode5_shift_button.center().x() <= startPos[5].x + range &&
					startPos[5].y - range <= INTmode5_shift_button.center().y() && INTmode5_shift_button.center().y() <= startPos[5].y + range)
				{
					return;
				}
				if (startPos[6].x - range <= INTmode6_shift_button.center().x() && INTmode6_shift_button.center().x() <= startPos[6].x + range &&
					startPos[6].y - range <= INTmode6_shift_button.center().y() && INTmode6_shift_button.center().y() <= startPos[6].y + range)
				{
					return;
				}
				
				// almost stationary (small move) but additional touch
				if (fingerTouchNum > -fingerTouchMode)
				{
					fingerTouchMode = -fingerTouchNum;
					return;
				}
				
			}
			else
			{
				if (fingerTouchNum > -fingerTouchMode)
				{
					fingerTouchMode = -fingerTouchNum;
					return;
				}
			}
			///////////
		}
}

void
ScribbleArea::recordPoints(const QTouchEvent::TouchPoint touchPoint)
{
	int fingerID = touchPoint.id() % MAX_TOUCH_NUM;

	finger_touch_or_not[fingerID] = 1;

	getFingerTouchNum();

	startPos[fingerID].x = touchPoint.startPos().x();
	startPos[fingerID].y = touchPoint.startPos().y();

	currentPos[fingerID].x = touchPoint.lastPos().x();
	currentPos[fingerID].y = touchPoint.lastPos().y();

	VOL_VECTOR2D displacement;

	displacement.x = currentPos[fingerID].x - startPos[fingerID].x;
	displacement.y = currentPos[fingerID].y - startPos[fingerID].y;

	float disp = VOL_VectorLength2D(&displacement);

	//printf("%d: (%.2f,%2f) -> (%.2f,%.2f) disp=%.3f\n",fingerID, startPos[fingerID].x, startPos[fingerID].y, currentPos[fingerID].x, currentPos[fingerID].y,disp);

	if (disp > max_displacement_of_each_finger[fingerID])	 max_displacement_of_each_finger[fingerID] = disp;

	if (disp > max_displacement_among_fingers)	 max_displacement_among_fingers = disp;
}


void
ScribbleArea::sort4points()	// only for 4 fingers mode
{
	if (pointsSorted == true)	return;

	sortedIndices[0] = 0;
	
	int closestTo0 = 1;
	float minDistanceTo0 = get_distance_2D( &(startPos[0]), &(startPos[1]) );

	for (int i = 2; i < 4; i++)
	{
		float distance = get_distance_2D(&(startPos[0]), &(startPos[i]));
		
		if (distance < minDistanceTo0)
		{
			closestTo0 = i;
			minDistanceTo0 = distance;
		}
	}
	sortedIndices[1] = closestTo0;

	switch (closestTo0)
	{
		case 1:	sortedIndices[2] = 2;	sortedIndices[3] = 3;	break;
		case 2:	sortedIndices[2] = 1;	sortedIndices[3] = 3;	break;
		case 3:	sortedIndices[2] = 1;	sortedIndices[3] = 2;	break;
		default:	break;
	}

	pointsSorted = true;
}



void
ScribbleArea::resetPoints()	//触った箇所などのリセット	
{
	for (int i = 0; i < MAX_TOUCH_NUM; i++)
	{
		finger_touch_or_not[i] = 0;
		max_displacement_of_each_finger[i] = 0;
	}

	max_displacement_among_fingers = 0;

	fingerTouchNum = 0;

	fingerTouchMode = MODE_NO_FINGER;

	pointsSorted = false;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//int counter = 1;
//int counter1 = 1;
//int counter2 = 0;
//void
//ScribbleArea::getDistanceFromCenterSphere()
//{
//
//	VOL_VECTOR3D center_sphere[3];
//
//	center_sphere[0].x = SPHERE1_x; center_sphere[0].y = SPHERE1_y; center_sphere[0].z = SPHERE1_z;  //sphere1
//	center_sphere[1].x = SPHERE2_x; center_sphere[1].y = SPHERE2_y; center_sphere[1].z = SPHERE2_z;  //sphere2
//	center_sphere[2].x = SPHERE3_x; center_sphere[2].y = SPHERE3_y; center_sphere[2].z = SPHERE3_z;  //sphere3
//
//	VOL_VECTOR3D Po_from_center_sphere[3];
//	VOL_VECTOR3D HofPo[3], H_from_center_sphere[3];
//	VOL_VECTOR3D NormalVector;
//	VOL_VECTOR3D VerticalVector[3];
//	float distance[3], t[3];
//
//	VOL_OuterProduct3D(&u, &v, &NormalVector);
//	VOL_NormalizeVector3D(&NormalVector);
//
//	for (int i = 0; i < 3; i++) 
//	{
//		subtract_vector_3D(&center_sphere[i], &Po, &Po_from_center_sphere[i]);
//		t[i] = VOL_InnerProduct3D(&Po_from_center_sphere[i], &NormalVector);
//	
//		if (t[i] < 0) //When the normal vector is not in the plane direction
//		{	
//			t[i] *= -1;
//		}
//
//		VerticalVector[i].x = NormalVector.x * t[i];
//		VerticalVector[i].y = NormalVector.y * t[i];
//		VerticalVector[i].z = NormalVector.z * t[i];
//
//		add_vector_3D(&center_sphere[i], &VerticalVector[i], &HofPo[i]);
//		subtract_vector_3D(&HofPo[i], &center_sphere[i], &H_from_center_sphere[i]);
//
//		distance[i] = VOL_VectorLength3D(&VerticalVector[i]);
//	}
//
//	if (distance[0] == DISTANCE1 && distance[1] == DISTANCE2 && distance[2] == DISTANCE3)
//	{
//		this->resetTimer();
//
//		if (counter2 % 2 == 0) {
//			printf("no started\n");
//			counter2++;
//		}
//
//		if (counter % 3 == 0) 
//		{
//			counter++;
//		}
//	}
//	else if (counter % 3 == 1)
//	{
//		printf("[%d]started\n",counter1);
//		counter++;
//	}
//	else if (distance[0] < 2 && distance[1] < 2 && distance[2] < 2 && counter %3 == 2)
//	{
//		getSphereTime = getTimeInMillisec();
//		printf("getSphereTime = %3.4f(sec)\n", getSphereTime / 1000);
//		counter++;
//		counter1++;		//times
//		counter2++;
//	}
//
//
//
//	//printf("distance1 = %lf, distance2 = %lf, distance3 = %lf\n", distance[0], distance[1], distance[2]);
//
//}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void
ScribbleArea::process1fingerMode()
{
	slideSlice(	currentPos[0].x - startPos[0].x, currentPos[0].y - startPos[0].y );
	setResiliceImage();

	/*
	QRectF rect = touchPoint.rect();
	if (rect.isEmpty()) {
		qreal diameter = 10;
		rect.setSize(QSizeF(diameter, diameter));
	}

	QPainter painter(this);
	painter.begin(this);
	painter.setPen(Qt::NoPen);
	painter.setBrush(myPenColors.at(touchPoint.id() % myPenColors.count()));
	painter.drawEllipse(rect);
	painter.end();
	*/
}


void
ScribbleArea::process2fingerMode()
{
	VOL_VECTOR2D start1to0, current1to0;
	float	theta, scale;

	subtract_vector_2D( &(startPos[0]), &(startPos[1]), &start1to0);
	subtract_vector_2D( &(currentPos[0]), &(currentPos[1]), &current1to0);
	
	get_changes_of_vector_angle_and_length_2D(&start1to0,&current1to0,&theta,&scale);

	pinchSlice(theta, scale, &(startPos[0]), &(currentPos[0]) );
	setResiliceImage();
}


void
ScribbleArea::process3fingerMode()
{
	pushSlice( qMax( qMax(currentPos[0].y-startPos[0].y, currentPos[1].y-startPos[1].y),currentPos[2].y-startPos[2].y )*psize );
	setResiliceImage();
}


void
ScribbleArea::process3fingerTMPMode()
{
	top_pos = startPos[0];
	for (int i = 1; i < 3; i++)
	{
		if (startPos[i].y < top_pos.y)
		{
			top_pos = startPos[i];
		}
	}

	int range = BUTTON_DIAMETER/2;

	//if (startPos[3].x - range <= INTmode3_shift_button.center().x() && INTmode3_shift_button.center().x() <= startPos[3].x + range && startPos[3].y - range <= INTmode3_shift_button.center().y() && INTmode3_shift_button.center().y() <= startPos[3].y + range) //An area of button to exit INTmode
	//{
	//	fingerTouchMode = MODE_INT_3_FINGERS; //MODE_INT_3_FINGERS confirmed
	//}
}



void
ScribbleArea::process3fingerINTMode()
{
	int range = BUTTON_DIAMETER/2;
	float distance = 0;

	if (currentPos[0].x - range <= rect_b.center().x() && rect_b.center().x() <= currentPos[0].x + range && currentPos[0].y - range <= rect_b.center().y() && rect_b.center().y() <= currentPos[0].y + range) //An area of button to exit INTmode
	{
		fingerTouchMode = MODE_NO_FINGER;
		startPos[3].x = 0; startPos[3].y = 0; //startPos[3] of TMPmode reset
	}
	else 
	{
		switch (fingerTouchNum)
		{
			case MODE_1_FINGER:		distance = (currentPos[0].y - startPos[0].y) * psize;		 break;
			case MODE_2_FINGERS:	distance = (qMax(currentPos[0].y - startPos[0].y, currentPos[1].y - startPos[1].y) / 2) * psize;		break;
			case MODE_3_FINGERS:	distance = (qMax(qMax(currentPos[0].y - startPos[0].y, currentPos[1].y - startPos[1].y), currentPos[2].y - startPos[2].y) / 3) * psize; 	break;
			case MODE_4_FINGERS:	distance = (qMax(qMax(qMax(currentPos[0].y - startPos[0].y, currentPos[1].y - startPos[1].y), currentPos[2].y - startPos[2].y), currentPos[3].y - startPos[3].y) / 4) * psize;	break;
			case MODE_5_FINGERS:	distance = (qMax(qMax(qMax(qMax(currentPos[0].y - startPos[0].y, currentPos[1].y - startPos[1].y), currentPos[2].y - startPos[2].y), currentPos[3].y - startPos[3].y), currentPos[4].y - startPos[4].y) / 5) * psize;	break;

			default:
				break;
		}

		pushSlice(distance);
		setResiliceImage();
	}
}


void
ScribbleArea::process4fingerMode()
{
	VOL_VECTOR2D pair1midPnt, pair2midPnt;
	float	pair1rotAngle, pair2rotAngle, pair1RightScrewAngle;

	getSliceRotationGeometry( &pair1midPnt, &pair2midPnt, &pair1rotAngle, &pair2rotAngle );

	pair1RightScrewAngle = (pair1rotAngle + (-pair2rotAngle)) / 2.0f;

	rotateSlice( &pair1midPnt, &pair2midPnt, pair1RightScrewAngle );
	setResiliceImage();
}


void
ScribbleArea::process4fingerTMPMode()
{
	// sort by distances to the first touch point
	sort4points();

	if(startPos[sortedIndices[0]].x < startPos[sortedIndices[3]].x)
	{
		if (startPos[sortedIndices[2]].y < startPos[sortedIndices[3]].y)
		{
			upper_right_pos = startPos[sortedIndices[2]];
		}
		else
		{
			upper_right_pos = startPos[sortedIndices[3]];
		}
	}
	else 
	{
		if (startPos[sortedIndices[0]].y < startPos[sortedIndices[1]].y)
		{
			upper_right_pos = startPos[sortedIndices[0]];
		}
		else 
		{
			upper_right_pos = startPos[sortedIndices[1]];
		}
	}

	int range = BUTTON_DIAMETER/2;

	//An area of button to exit INTmode
	if (startPos[4].x - range <= INTmode4_shift_button.center().x() && INTmode4_shift_button.center().x() <= startPos[4].x + range &&
		startPos[4].y - range <= INTmode4_shift_button.center().y() && INTmode4_shift_button.center().y() <= startPos[4].y + range)
		
	{
		fingerTouchMode = MODE_INT_4_FINGERS; //MODE_INT_4_FINGERS confirmed
	}
}


void
ScribbleArea::process4fingerINTMode()
{
	VOL_VECTOR2D pair1midPnt, pair2midPnt;

	// sort by distances to the first touch point
	sort4points();

	// 2 mid-points fore rotation axis
	get_midpoint_2D(&(startPos[sortedIndices[0]]), &(startPos[sortedIndices[1]]), &pair1midPnt);
	get_midpoint_2D(&(startPos[sortedIndices[2]]), &(startPos[sortedIndices[3]]), &pair2midPnt);

	if (line_start_x == 0 && line_start_y == 0 && line_end_x == 0 && line_end_y == 0)
	{
		line_start_x = midPnt1.x = tem_midPnt1.x = pair1midPnt.x;
		line_start_y = midPnt1.y = tem_midPnt1.y = pair1midPnt.y;
		line_end_x = midPnt2.x = tem_midPnt2.x = pair2midPnt.x;
		line_end_y = midPnt2.y = tem_midPnt2.y = pair2midPnt.y;
	}

	int range = BUTTON_DIAMETER/2;

	if (judg_s_x != startPos[0].x && judg_s_y != startPos[0].y) //startpos changed
	{
		tem_midPnt1 = midPnt1;
		tem_midPnt2 = midPnt2;
	}

	judg_s_x = startPos[0].x;
	judg_s_y = startPos[0].y;

	//An area where the rotation axis can move
	if (currentPos[0].x - range <= rect_s.center().x() && rect_s.center().x() <= currentPos[0].x + range &&
		currentPos[0].y - range <= rect_s.center().y() && rect_s.center().y() <= currentPos[0].y + range) 
	{
		subtract_vector_2D(&tem_midPnt1, &startPos[0], &sub_vector);
		add_vector_2D(&currentPos[0], &sub_vector, &midPnt1);

		line_start_x = midPnt1.x;
		line_start_y = midPnt1.y;
	}
	else //An area where the rotation axis can move
	if (currentPos[0].x - range <= rect_e.center().x() && rect_e.center().x() <= currentPos[0].x + range &&
		currentPos[0].y - range <= rect_e.center().y() && rect_e.center().y() <= currentPos[0].y + range) 
	{
		subtract_vector_2D(&tem_midPnt2, &startPos[0], &sub_vector);
		add_vector_2D(&currentPos[0], &sub_vector, &midPnt2);

		line_end_x = midPnt2.x;
		line_end_y = midPnt2.y;
	}
	else //An area of button to exit INTmode
	if (currentPos[0].x - range <= rect_b.center().x() && rect_b.center().x() <= currentPos[0].x + range &&
		currentPos[0].y - range <= rect_b.center().y() && rect_b.center().y() <= currentPos[0].y + range) 
	{
		fingerTouchMode = MODE_NO_FINGER;
		line_start_x = 0; line_start_y = 0; line_end_x = 0; line_end_y = 0; //drawline reset
		startPos[4].x = 0; startPos[4].y = 0; //startPos[4] of TMPmode reset
	}
	else
	{
		VOL_VECTOR2D rotAxisDir, Axis;
		VOL_VECTOR2D HofPo, MofPo, vec_Spo_M;

		subtract_vector_2D(&midPnt2, &midPnt1, &rotAxisDir);
		VOL_NormalizeVector2D(&rotAxisDir);

		get_foot_of_perpendicular_2D(&startPos[0], &rotAxisDir, &midPnt1, &HofPo);

		subtract_vector_2D(&startPos[0], &HofPo, &Axis);
		VOL_NormalizeVector2D(&Axis);

		get_foot_of_perpendicular_2D(&currentPos[0], &Axis, &startPos[0], &MofPo);

		subtract_vector_2D(&MofPo, &startPos[0], &vec_Spo_M);

		float len_Spo_M;
		float ScrewAngle;

		len_Spo_M = VOL_VectorLength2D(&vec_Spo_M);

		if (vec_Spo_M.y < 0)
		{
			len_Spo_M *= REVERSE_ROTATION; //Reverse the direction of rotation
		}
		else if(vec_Spo_M.y = 0)
		{
			if (vec_Spo_M.x > 0) 
			{
				len_Spo_M *= REVERSE_ROTATION; //Reverse the direction of rotation
			}
		}

		switch (fingerTouchNum)
		{
			case MODE_1_FINGER:		ScrewAngle = REVERSE_ROTATION * (M_PI*0.125f) * len_Spo_M / DISPLAY_HEIGHT;	break;
			case MODE_2_FINGERS:	ScrewAngle = REVERSE_ROTATION * (M_PI*0.250f) * len_Spo_M / DISPLAY_HEIGHT;	break;
			case MODE_3_FINGERS:	ScrewAngle = REVERSE_ROTATION * (M_PI*0.500f) * len_Spo_M / DISPLAY_HEIGHT;	break;
			case MODE_4_FINGERS:	ScrewAngle = REVERSE_ROTATION * (M_PI*1.000f) * len_Spo_M / DISPLAY_HEIGHT;	break;
			case MODE_5_FINGERS:	ScrewAngle = REVERSE_ROTATION * (M_PI*2.000f) * len_Spo_M / DISPLAY_HEIGHT;	break;

			default:
				break;
		}

		rotateSlice(&midPnt1, &midPnt2, ScrewAngle);
		setResiliceImage();

		
		//printf("fingerTouchNum = %d\n",fingerTouchNum);
		//printf("H = (%lf , %lf)\n", HofPo.x, HofPo.y);
		//printf("len_Spo_M = %lf\n", len_Spo_M);
		//printf("currentPos[0] = (%lf,%lf), startPos[0] = (%lf,%lf)\n", currentPos[0].x, currentPos[0].y, startPos[0].x, startPos[0].y);
		//printf("midPnt1 = (%lf,%lf) , midPnt2 = (%lf,%lf)\n", midPnt1.x, midPnt1.y, midPnt2.x, midPnt2.y);
		//printf("tem_midPnt1 = (%lf,%lf)   , tem_midPnt2 = (%lf,%lf)\n", tem_midPnt1.x, tem_midPnt1.y, tem_midPnt2.x, tem_midPnt2.y);
		//printf("rect_s = (%lf,%lf) , rect_e = (%lf,%lf)\n", rect_s.center().x(), rect_s.center().y(), rect_e.center().x(), rect_e.center().y());
	}
}


void
ScribbleArea::process5fingerMode()
{
	changeGreyscale((float)(currentPos[0].x - startPos[0].x), (float)(currentPos[0].y - startPos[0].y));
	setResiliceImage();
}


void
ScribbleArea::process5fingerTMPMode()
{
	top_pos = startPos[0];
	for (int i = 1; i < 5; i++)
	{
		if (startPos[i].y < top_pos.y)
		{
			top_pos = startPos[i];
		}
	}

	int range = BUTTON_DIAMETER/2;

	//if (startPos[5].x - range <= INTmode5_shift_button.center().x() && INTmode5_shift_button.center().x() <= startPos[5].x + range && startPos[5].y - range <= INTmode5_shift_button.center().y() && INTmode5_shift_button.center().y() <= startPos[5].y + range) //An area of button to exit INTmode
	//{
	//	fingerTouchMode = MODE_INT_5_FINGERS; //MODE_INT_5_FINGERS confirmed
	//}
}


void
ScribbleArea::process5fingerINTMode()
{
	int range = BUTTON_DIAMETER/2;

	if (currentPos[0].x - range <= rect_b.center().x() && rect_b.center().x() <= currentPos[0].x + range && currentPos[0].y - range <= rect_b.center().y() && rect_b.center().y() <= currentPos[0].y + range) //An area of button to exit INTmode
	{
		fingerTouchMode = MODE_NO_FINGER;
		startPos[5].x = 0; startPos[5].y = 0; //startPos[5] of TMPmode reset
	}
	else
	{
		changeGreyscale((float)(currentPos[0].x - startPos[0].x), (float)(currentPos[0].y - startPos[0].y));
		setResiliceImage();
	}
}


void
ScribbleArea::process6fingerMode()
{
	VOL_RAYCASTER;
	setResiliceImage();
}


void
ScribbleArea::process6fingerTMPMode()
{


	sortedIndices2[0] = 0;

	int closestTo0 = 1;
	int closestTo1 = 2;
	float exchangef = 0;
	float minDistanceTo0 = get_distance_2D(&(startPos[0]), &(startPos[1]));
	float minDistanceTo1 = get_distance_2D(&(startPos[0]), &(startPos[2]));

	if (minDistanceTo0 > minDistanceTo1) {
		exchangef = minDistanceTo0;
		minDistanceTo0 = minDistanceTo1;
		minDistanceTo1 = exchangef;
		closestTo0 = 2;
		closestTo1 = 1;
	}
	for (int i = 3; i < 6; i++)
	{
		float distance = get_distance_2D(&(startPos[0]), &(startPos[i]));

		if (distance < minDistanceTo0)
		{
			closestTo1 = closestTo0;
			minDistanceTo1 = minDistanceTo0;
			closestTo0 = i;
			minDistanceTo0 = distance;

		}
		else if (distance < minDistanceTo1)
		{
			closestTo1 = i;
			minDistanceTo1 = distance;
		}
	}

	sortedIndices2[1] = closestTo0;
	sortedIndices2[2] = closestTo1;

	int x = 3;
	for (int i = 1; i < 6; i++) {
		if (closestTo0 != i && closestTo1 != i) {
			sortedIndices2[x] = i;
			x++;
		}
	}
	pointsSorted = true;

	VOL_VECTOR2D p1, p2;
	VOL_VECTOR2D midpoint;
	p1.x = p1.y = p2.x = p2.y = 0;
	midpoint.x = midpoint.y = 0;

	circleOf3Point(startPos[sortedIndices2[0]].x, startPos[sortedIndices2[0]].y, startPos[sortedIndices2[1]].x, startPos[sortedIndices2[1]].y, startPos[sortedIndices2[2]].x, startPos[sortedIndices2[2]].y, &p1.x, &p1.y);
	circleOf3Point(startPos[sortedIndices2[3]].x, startPos[sortedIndices2[3]].y, startPos[sortedIndices2[4]].x, startPos[sortedIndices2[4]].y, startPos[sortedIndices2[5]].x, startPos[sortedIndices2[5]].y, &p2.x, &p2.y);
	
	get_midpoint_2D(&p1, &p2, &midpoint);
	top_pos = midpoint;

	int range = BUTTON_DIAMETER / 2;

}




void
ScribbleArea::process10fingerMode()
{
	initializeViewParams();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


bool
ScribbleArea::processTouchEvent(QEvent *event)
{
	switch (event->type())
	{
		case QEvent::TouchBegin:
		case QEvent::TouchUpdate:
		{
			if (event->type()== QEvent::TouchBegin)
			{
				//printf("======== Touch Begin ========\n");
				memoryPreviousViewParams();
			}

			QList<QTouchEvent::TouchPoint> touchPoints = static_cast<QTouchEvent *>(event)->touchPoints();

			resliceLQ = true;

			foreach(const QTouchEvent::TouchPoint &touchPoint, touchPoints)
			{
				recordPoints(touchPoint);

				updateFingerTouchMode(touchPoint);

				//if (touchPoint.state() == Qt::TouchPointStationary )	resliceLQ = false;

				switch (this->fingerTouchMode)
				{
					case MODE_1_FINGER:			process1fingerMode();				break;
					case MODE_2_FINGERS:		process2fingerMode();				break;
					case MODE_3_FINGERS:		process3fingerMode();				break;
					case MODE_TMP_3_FINGERS:	process3fingerTMPMode();			break;
					case MODE_INT_3_FINGERS:    process3fingerINTMode();			break;
					case MODE_4_FINGERS:		process4fingerMode();				break;
					case MODE_TMP_4_FINGERS:	process4fingerTMPMode();			break;
					case MODE_INT_4_FINGERS:	process4fingerINTMode();			break;
					case MODE_5_FINGERS:		process5fingerMode();				break;
					case MODE_TMP_5_FINGERS:	process5fingerTMPMode();			break;
					case MODE_INT_5_FINGERS:	process5fingerINTMode();			break;
					case MODE_6_FINGERS:        process6fingerMode();				break;
					case MODE_TMP_6_FINGERS:	process6fingerTMPMode();			break;

					default:						break;
				}

				//getDistanceFromCenterSphere();


				update();
			}
			break;
		}
		case QEvent::TouchEnd:
		{
			QTouchEvent * touchEvent = static_cast<QTouchEvent *>(event);
			QList<QTouchEvent::TouchPoint> touchPoints = touchEvent->touchPoints();

			foreach(const QTouchEvent::TouchPoint &touchPoint, touchPoints)
			{
				recordPoints(touchPoint);

				resliceLQ = false;

				switch (this->fingerTouchMode)
				{
					case MODE_10_FINGERS:	process10fingerMode();	break;
					default:										break;
				}

				setResiliceImage();
				update();

				if (fingerTouchMode == MODE_INT_3_FINGERS)
				{
					resetPoints();
					fingerTouchMode = MODE_INT_3_FINGERS;
				}
				else if (fingerTouchMode == MODE_INT_4_FINGERS)
				{
					resetPoints();
					fingerTouchMode = MODE_INT_4_FINGERS;
				}
				else if (fingerTouchMode == MODE_INT_5_FINGERS)
				{
					resetPoints();
					fingerTouchMode = MODE_INT_5_FINGERS;
				}
				else
				{
					resetPoints();
				}

				//////////////////////////////////////////////////////showTimeStatistics();

				//printf("-------- Touch End --------\n");
			}
			break;
		}
		default:	break;
	}
	return true;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static int tmpcnt = 0;
bool
ScribbleArea::eventFilter(QObject *obj, QEvent *event)
{
	// for idle event handling
	//printf("aaaaaaa %d\n",tmpcnt++);
	//if (fingerTouchMode != MODE_TMP_3_FINGERS && fingerTouchMode != MODE_TMP_4_FINGERS && fingerTouchMode != MODE_TMP_5_FINGERS)
	//{
	//	this->resetTimer();
	//}

	//if (fingerTouchMode == MODE_TMP_3_FINGERS)
	//{
	//	INTTime = getTimeInMillisec();
	//	printf("INTTime = %3.4f(msec)\n", INTTime);
	//	if (INTTime >= 1000)
	//	{
	//		fingerTouchMode *= -10; //MODE_INT_3_FINGERS confirmed
	//		process3fingerINTMode();
	//		update();
	//	}
	//}

	//if (fingerTouchMode == MODE_TMP_4_FINGERS)
	//{
	//	INTTime = getTimeInMillisec();
	//	printf("INTTime = %3.4f(msec)\n", INTTime);
	//	if (INTTime >= 1000)
	//	{
	//		fingerTouchMode *= -10; //MODE_INT_4_FINGERS confirmed
	//		process4fingerINTMode();
	//		update();
	//	}
	//}

	//if (fingerTouchMode == MODE_TMP_5_FINGERS)
	//{
	//	INTTime = getTimeInMillisec();
	//	printf("INTTime = %3.4f(msec)\n", INTTime);
	//	if (INTTime >= 1000)
	//	{
	//		fingerTouchMode *= -10; //MODE_INT_5_FINGERS confirmed
	//		process5fingerINTMode();
	//		update();
	//	}
	//}

	return false;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void
ScribbleArea::resetTimer()
{
	GetSystemTimeAsFileTime(&startTime);
}

double
ScribbleArea::getTimeInMillisec()
{
	FILETIME currentTimne;

	GetSystemTimeAsFileTime(&currentTimne);											//現在の時間を取得

	return ((double)GetFileTimeDifference(&currentTimne, &startTime)) / 1000000.0;	//（現在の時間ー始めた時間）*100/1000000を返す
}

void
ScribbleArea::showTime(char* string)
{
	FILETIME currentTimne;

	GetSystemTimeAsFileTime(&currentTimne);

	printf("%s (msec): %3.4lf\n",string,((double)GetFileTimeDifference(&currentTimne, &startTime)) / 1000000.0f);
}

void
ScribbleArea::showTimeStatistics()
{
	FILETIME currentTimne;

	GetSystemTimeAsFileTime(&currentTimne);

	printf("Average times: \n");
	printf("	LQ Reslice (msec): %3.4lf\n", LQresliceTime / (double)LQresliceCount);
	printf("	LQ Convert (msec): %3.4lf\n", LQconvertTime / (double)LQconvertCount);
	printf("	HQ Reslice (msec): %3.4lf\n", HQresliceTime / (double)HQresliceCount);
	printf("	HQ Convert (msec): %3.4lf\n", HQconvertTime / (double)HQconvertCount);
	printf("	Draw (msec): %3.4lf\n",drawTime / (double)drawCount);

	LQresliceTime = LQconvertTime = HQresliceTime = HQconvertTime = drawTime = 0;
	LQresliceCount = LQconvertCount = HQresliceCount = HQconvertCount = drawCount = 0;
}

void
ScribbleArea::circleOf3Point(float x1, float  y1, float x2, float y2, float x3, float y3, float *cx, float *cy)
{
	float 	ox, oy, a, b, c, d;
	float 	r1, r2, r3;


	a = x2 - x1;
	b = y2 - y1;
	c = x3 - x1;
	d = y3 - y1;


	if ((a && d) || (b && c)) {
		ox = x1 + (d * (a * a + b * b) - b * (c * c + d * d)) / (a * d - b * c) / 2;
		if (b) {
			oy = (a * (x1 + x2 - ox - ox) + b * (y1 + y2)) / b / 2;
		}
		else {
			oy = (c * (x1 + x3 - ox - ox) + d * (y1 + y3)) / d / 2;
		}
		*cx = ox;
		*cy = oy;
	}


}