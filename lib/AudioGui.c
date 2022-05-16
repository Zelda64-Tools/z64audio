#include "AudioGui.h"
#include "AudioConvert.h"

SplitTask gTaskTable[2] = {
	{
		"None",
	},
	{
		"Sampler",
		DefineTask(Sampler)
	},
};

Sampler* __sampler;

// !! //

void Window_DropCallback(GLFWwindow* window, s32 count, char* item[]) {
	if (count == 1) {
		if (StrEndCase(item[0], ".wav")) {
			SoundFormat fmt;
			
			if (__sampler == NULL)
				return;
			
			if (__sampler->player) {
				Sound_Free(__sampler->player);
				Audio_FreeSample(&__sampler->sample);
			}
			
			Audio_InitSample(&__sampler->sample, item[0], 0);
			Log("Input File [%s]", item[0]);
			Audio_LoadSample(&__sampler->sample);
			
			fmt = __sampler->sample.bit == 16 ? SOUND_S16 : (__sampler->sample.bit == 32 && __sampler->sample.dataIsFloat) ? SOUND_F32 : SOUND_S32;
			
			__sampler->player = Sound_Init(fmt, __sampler->sample.sampleRate, __sampler->sample.channelNum, Audio_Playback, &__sampler->sample);
			
			if (StrStr(item[0], "Sample.wav")) {
				char* ptr = StrStr(item[0], "\\Sample.wav");
				char* str;
				if (ptr == NULL) StrStr(item[0], "/Sample.wav");
				if (ptr == NULL)
					strncpy(__sampler->sampleName.txt, String_GetBasename(item[0]), 64);
				else {
					str = ptr;
					
					while (str[-1] != '/' && str[-1] != '\\' && str[-1] != '\0')
						str--;
					
					strncpy(__sampler->sampleName.txt, str, (uPtr)ptr - (uPtr)str);
				}
			} else
				strncpy(__sampler->sampleName.txt, String_GetBasename(item[0]), 64);
			
			__sampler->loopA.max = (f32)__sampler->sample.samplesNum - 0.95;
			__sampler->loopB.max = (f32)__sampler->sample.samplesNum;
			
			if (__sampler->sample.instrument.loop.count) {
				Element_Slider_SetValue(&__sampler->loopA, __sampler->sample.instrument.loop.start);
				Element_Slider_SetValue(&__sampler->loopB, __sampler->sample.instrument.loop.end);
			} else {
				Element_Slider_SetValue(&__sampler->loopA, 0);
				Element_Slider_SetValue(&__sampler->loopB, 0);
			}
		}
	}
}

void Window_Update(WindowContext* winCtx) {
	GeoGrid_Update(&winCtx->geoGrid);
	Cursor_Update(&winCtx->cursor);
}

void Window_Draw(WindowContext* winCtx) {
	GeoGrid_Draw(&winCtx->geoGrid);
}

// !! //

void Sampler_Init(WindowContext* winCtx, Sampler* this, Split* split) {
	__sampler = this;
	
	this->playButton.txt = "Play Sample";
	this->playButton.toggle = true;
	
	this->setLoopButton.txt = "Set";
	this->clearLoopButton.txt = "Clear";
	
	this->sampleName.txt = Calloc(0, 66);
	this->sampleName.size = 64;
	
	this->textLoop.txt = "Loop Settings";
	
	this->loopA.isInt = this->loopB.isInt = true;
	
	this->zoom.end = 1.0f;
	this->zoom.vEnd = 1.0f;
	
	this->visual.findCol = Theme_GetColor(THEME_HIGHLIGHT, 25, 1.0);
}

void Sampler_Destroy(WindowContext* winCtx, Sampler* this, Split* split) {
	Free(this->sampleName.txt);
	
	if (this->player) {
		Sound_Free(this->player);
		Audio_FreeSample(&this->sample);
	}
}

void Sampler_Update(WindowContext* winCtx, Sampler* this, Split* split) {
	AudioSample* sample = &this->sample;
	s32 y = SPLIT_ELEM_X_PADDING;
	f32 sliderA;
	f32 sliderB;
	
	// For drag n dropping
	if (split->mouseInSplit)
		__sampler = this;
	
	this->loopA.locked = false;
	this->loopB.locked = false;
	if (this->sample.audio.p == NULL) {
		this->loopA.locked = true;
		this->loopB.locked = true;
	}
	
	Element_SetRect_Multiple(
		split,
		y,
		3,
		&this->playButton.rect,
		0.20,
		NULL,
		0.20,
		&this->sampleName.rect,
		0.60
	);
	
	if (winCtx->input.key[KEY_SPACE].press) {
		if (sample->doPlay == 1) {
			sample->doPlay = 0;
			this->playButton.toggle = 1;
		} else if (sample->doPlay == 0)
			this->playButton.toggle = 2;
	}
	
	if (Element_Button(&winCtx->geoGrid, split, &this->playButton)) {
		if (sample->doPlay == 0) {
			sample->doPlay = 1;
		}
		
		if (sample->doPlay < 0) {
			this->playButton.toggle = 1;
			sample->playFrame = 0;
			sample->doPlay = 0;
		}
		
		this->butTog = true;
	} else {
		if (this->butTog) {
			sample->doPlay = false;
			this->butTog = false;
		}
	}
	
	Element_Textbox(&winCtx->geoGrid, split, &this->sampleName);
	
	y += SPLIT_TEXT_H + SPLIT_ELEM_X_PADDING;
	
	Element_SetRect_Multiple(
		split,
		y,
		5,
		&this->textLoop.rect,
		0.20,
		&this->loopA.rect,
		0.20,
		&this->loopB.rect,
		0.20,
		&this->setLoopButton.rect,
		0.20,
		&this->clearLoopButton.rect,
		0.20
	);
	
	if (Element_Button(&winCtx->geoGrid, split, &this->clearLoopButton)) {
		sample->instrument.loop.start = 0;
		sample->instrument.loop.end = 0;
		sample->instrument.loop.count = 0;
		
		Element_Slider_SetValue(&this->loopA, sample->instrument.loop.start);
		this->loopB.min = sliderA + 1.0f;
		Element_Slider_SetValue(&this->loopB, sample->instrument.loop.end);
	}
	
	if (Element_Button(&winCtx->geoGrid, split, &this->setLoopButton)) {
		if (sample->selectStart && sample->selectEnd) {
			sample->instrument.loop.start = sample->selectStart;
			sample->instrument.loop.end = sample->selectEnd;
			sample->instrument.loop.count = -1;
			
			sample->selectStart = sample->selectEnd = 0;
			
			Element_Slider_SetValue(&this->loopA, sample->instrument.loop.start);
			this->loopB.min = sliderA + 1.0f;
			Element_Slider_SetValue(&this->loopB, sample->instrument.loop.end);
		}
	}
	
	sliderA = Element_Slider(&winCtx->geoGrid, split, &this->loopA);
	if (this->loopA.holdState) {
		this->loopB.min = sliderA + 1.0f;
		Element_Slider_SetValue(&this->loopB, this->prevLoopB);
	}
	sliderB = Element_Slider(&winCtx->geoGrid, split, &this->loopB);
	this->prevLoopB = sliderB;
	
	Element_Text(&winCtx->geoGrid, split, &this->textLoop);
	
	if (this->loopA.holdState || this->loopB.holdState) {
		if (this->loopA.value != 0 || this->loopB.value != 0) {
			sample->instrument.loop.start = sliderA;
			sample->instrument.loop.end = sliderB;
			sample->instrument.loop.count = -1;
		} else {
			sample->instrument.loop.count = 0;
		}
	}
	
	this->waveFormPos = y + SPLIT_TEXT_H + SPLIT_ELEM_X_PADDING;
}

static f32 GetPos(f32 i, Rect* waverect, AudioSample* sample, Sampler* this) {
	f32 endSample = sample->samplesNum * this->zoom.vEnd;
	
	return waverect->x + waverect->w * ((i - sample->samplesNum * this->zoom.vStart) / (f32)(endSample - sample->samplesNum * this->zoom.vStart));
}

static void Sampler_Draw_Waveform_Line(void* vg, Rect* waverect, AudioSample* sample, Sampler* this) {
	f32 endSample = sample->samplesNum * this->zoom.vEnd;
	f32 smplPixelRatio = (endSample - sample->samplesNum * this->zoom.vStart) / waverect->w;
	f32 y;
	f32 x;
	f32 e = 0;
	f32 k = 0;
	s32 m = 0;
	
	nvgBeginPath(vg);
	nvgLineCap(vg, NVG_ROUND);
	nvgStrokeWidth(vg, 1.25f);
	
	for (s32 i = sample->samplesNum * this->zoom.vStart; i < endSample;) {
		s32 smpid = i;
		
		x = GetPos(i + k, waverect, sample, this);
		
		if (!IsBetween(x, waverect->x, waverect->x + waverect->w))
			goto adv;
		
		if (sample->bit == 16) {
			y = (f32)sample->audio.s16[smpid * sample->channelNum] / __INT16_MAX__;
		} else if (sample->dataIsFloat) {
			y = ((f32)sample->audio.f32[smpid * sample->channelNum]);
		} else {
			y = (f32)sample->audio.s32[smpid * sample->channelNum] / (f32)__INT32_MAX__;
		}
		
		if (m == 0) {
			m++;
			nvgMoveTo(
				vg,
				x,
				waverect->y + waverect->h * 0.5 - (waverect->h * 0.5) * y
			);
		} else {
			nvgLineTo(
				vg,
				x,
				waverect->y + waverect->h * 0.5 - (waverect->h * 0.5) * y
			);
		}
		
adv:
		k += smplPixelRatio / 8;
		if (!(k < 1.0))
			i++;
		k = WrapF(k, 0.0, 1.0);
	}
	
	nvgStrokeColor(vg, Theme_GetColor(THEME_HIGHLIGHT, 255, 0.95));
	nvgStroke(vg);
}

static void Sampler_Draw_Waveform_Block(void* vg, Rect* waverect, AudioSample* sample, Sampler* this) {
	s32 endSample = sample->samplesNum * this->zoom.vEnd;
	f32 smplStart = sample->samplesNum * this->zoom.vStart;
	f32 smplCount = (f32)(endSample - smplStart) / waverect->w;
	
	for (s32 i = 0; i < waverect->w; i++) {
		f64 yMin = 0;
		f64 yMax = 0;
		f64 y = 0;
		s32 e = smplStart + smplCount * i;
		
		for (s32 j = 0; j < smplCount; j++) {
			if (sample->bit == 16) {
				y = (f32)sample->audio.s16[(e + j) * sample->channelNum];
			} else if (sample->dataIsFloat) {
				y = ((f32)sample->audio.f32[(e + j) * sample->channelNum]);
			} else {
				y = (f32)sample->audio.s32[(e + j) * sample->channelNum];
			}
			
			yMax = Max(yMax, y);
			yMin = Min(yMin, y);
		}
		
		if (sample->bit == 16) {
			yMax /= __INT16_MAX__;
			yMin /= __INT16_MAX__;
		} else if (sample->dataIsFloat == false) {
			yMax /= __INT32_MAX__;
			yMin /= __INT32_MAX__;
		}
		
		nvgBeginPath(vg);
		nvgLineCap(vg, NVG_SQUARE);
		nvgStrokeWidth(vg, 1.75);
		
		nvgMoveTo(
			vg,
			waverect->x + i,
			waverect->y + waverect->h * 0.5 - (waverect->h * 0.5) * yMin
		);
		nvgLineTo(
			vg,
			waverect->x + i,
			waverect->y + waverect->h * 0.5 - (waverect->h * 0.5) * yMax + 1
		);
		
		nvgStrokeColor(vg, Theme_GetColor(THEME_HIGHLIGHT, 255, 0.95));
		nvgStroke(vg);
	}
}

static void Sampler_Draw_LoopMarker(void* vg, Rect* waverect, AudioSample* sample, Sampler* this) {
	if (sample->instrument.loop.count) {
		for (s32 j = 0; j < 2; j++) {
			f32 point = j == 0 ? this->visual.loopA : this->visual.loopB;
			f32 mul = j == 0 ? 1.0f : -1.0f;
			f32 y = waverect->y;
			Vec2f shape[] = {
				{ 8.0f, 0.0f },
				{ 0.0f, 8.0f },
				{ 0.0f, 0.0f },
			};
			
			nvgBeginPath(vg);
			nvgFillColor(vg, Theme_GetColor(THEME_RED, 255, 1.0));
			
			nvgMoveTo(
				vg,
				point,
				y
			);
			
			for (s32 i = 0; i < 3; i++) {
				nvgLineTo(
					vg,
					shape[i].x * mul + point,
					shape[i].y + y
				);
			}
			
			nvgFill(vg);
		}
	}
}

static void Sampler_Draw_Selection(void* vg, Rect* waverect, Rect* finder, AudioSample* sample, Sampler* this) {
	if (sample->selectStart == sample->selectEnd)
		return;
	
	nvgBeginPath(vg);
	nvgFillColor(vg, Theme_GetColor(THEME_PRIM, 127, 1.25));
	nvgRoundedRect(
		vg,
		this->visual.selectStart,
		waverect->y + 8,
		this->visual.selectStart != this->visual.selectEnd ? this->visual.selectEnd - this->visual.selectStart + 2 : 2,
		waverect->h - 8 * 2,
		SPLIT_ROUND_R
	);
	
	nvgFill(vg);
}

static void Sampler_Draw_Position(void* vg, Rect* waverect, Rect* finder, AudioSample* sample, Sampler* this) {
	nvgBeginPath(vg);
	nvgFillColor(vg, Theme_GetColor(THEME_PRIM, 255, 1.25));
	nvgRoundedRect(
		vg,
		this->visual.playPos,
		waverect->y + 8,
		2,
		waverect->h - 8 * 2,
		SPLIT_ROUND_R
	);
	
	nvgFill(vg);
}

static void Sampler_Draw_Grid(void* vg, Rect* waverect, AudioSample* sample, Sampler* this) {
	f32 endSample = sample->samplesNum * this->zoom.vEnd;
	f32 smplPixelRatio = (endSample - sample->samplesNum * this->zoom.vStart) / waverect->w;
	s32 o = 0;
	
	for (f32 i = 0; i < 1.0; i += 0.1) {
		f32 y = waverect->y + waverect->h * i;
		f32 a = powf(Abs(0.5 - i) * 2, 8);
		nvgBeginPath(vg);
		nvgLineCap(vg, NVG_ROUND);
		nvgStrokeWidth(vg, 1.25f);
		nvgStrokeColor(vg, Theme_GetColor(THEME_HIGHLIGHT, 50 - a * 35, 1.0));
		nvgMoveTo(vg, waverect->x + 2, y);
		nvgLineTo(vg, waverect->x + waverect->w - 4, y);
		nvgStroke(vg);
	}
	
	if (smplPixelRatio < 0.5) {
		for (s32 i = sample->samplesNum * this->zoom.vStart; i < endSample; i++) {
			f32 x = GetPos(i, waverect, sample, this);
			
			nvgBeginPath(vg);
			nvgLineCap(vg, NVG_ROUND);
			nvgStrokeWidth(vg, 1.25f);
			nvgStrokeColor(vg, Theme_GetColor(THEME_HIGHLIGHT, 25, 1.0));
			nvgMoveTo(vg, x, waverect->y + 2);
			nvgLineTo(vg, x, waverect->y + waverect->h - 4);
			nvgStroke(vg);
			o++;
		}
	}
}

static void Sampler_Draw_Finder(void* vg, Rect* waverect, Rect* finder, AudioSample* sample, Sampler* this) {
	nvgBeginPath(vg);
	nvgFillColor(vg, Theme_GetColor(THEME_BASE_L1, 255, 0.75));
	nvgRoundedRect(
		vg,
		waverect->x - 1,
		finder->y - 1,
		waverect->w + 2,
		finder->h + 2,
		SPLIT_ROUND_R * 2
	);
	nvgFill(vg);
	
	nvgBeginPath(vg);
	nvgFillColor(vg, this->visual.findCol);
	nvgRoundedRect(
		vg,
		finder->x,
		finder->y,
		finder->w,
		finder->h,
		SPLIT_ROUND_R * 2
	);
	nvgFill(vg);
	
	nvgBeginPath(vg);
	nvgFillColor(vg, Theme_GetColor(THEME_PRIM, 255, 1.0));
	nvgRoundedRect(
		vg,
		waverect->x + waverect->w * ((f32)sample->playFrame / sample->samplesNum) - 1,
		finder->y,
		2,
		finder->h,
		SPLIT_ROUND_R * 2
	);
	nvgFill(vg);
	
	if (sample->selectStart == sample->selectEnd)
		return;
	
	nvgBeginPath(vg);
	nvgFillColor(vg, Theme_GetColor(THEME_PRIM, 25, 1.25));
	nvgRoundedRect(
		vg,
		waverect->x + waverect->w * ((f32)sample->selectStart / sample->samplesNum) - 1,
		finder->y,
		waverect->w * (f32)(sample->selectEnd - sample->selectStart) / sample->samplesNum,
		finder->h,
		SPLIT_ROUND_R
	);
	
	nvgFill(vg);
}

void Sampler_Draw(WindowContext* winCtx, Sampler* this, Split* split) {
	void* vg = winCtx->vg;
	AudioSample* sample = &this->sample;
	Rect waverect = {
		SPLIT_ELEM_X_PADDING * 2,
		this->waveFormPos,
		split->rect.w - SPLIT_ELEM_X_PADDING * 4,
		split->rect.h - (this->waveFormPos + SPLIT_TEXT_H * 0.5 + SPLIT_BAR_HEIGHT) - 20
	};
	Rect finder = {
		waverect.x + ((waverect.w + 2) * this->zoom.start),
		waverect.y + waverect.h + 4,
		waverect.w * this->zoom.end - ((waverect.w - 4) * this->zoom.start),
		16,
	};
	u32 block = 0;
	f32 mouseX = Clamp((f32)(split->mousePos.x - waverect.x) / waverect.w, 0.0, 1.0);
	
	nvgBeginPath(vg);
	nvgFillPaint(
		vg,
		nvgBoxGradient(
			vg,
			waverect.x,
			waverect.y,
			waverect.w,
			waverect.h,
			0,
			8,
			Theme_GetColor(THEME_BASE_L1, 255, 1.05),
			Theme_GetColor(THEME_BASE_L1, 255, 0.45)
		)
	);
	nvgRoundedRect(
		vg,
		waverect.x,
		waverect.y,
		waverect.w,
		waverect.h,
		SPLIT_ROUND_R * 2
	);
	nvgFill(vg);
	
	if (this->sample.audio.p == NULL)
		return;
	
	if (GeoGrid_Cursor_InRect(split, &finder) || this->zoom.modifying) {
		block = true;
		
		Theme_SmoothStepToCol(&this->visual.findCol, Theme_GetColor(THEME_HIGHLIGHT, 175, 1.0), 0.25, 1.0, 0.0);
		
		if (winCtx->input.mouse.clickL.hold) {
			f32 xmod = (mouseX - this->mousePrevX);
			if (this->zoom.modifying == 0) {
				s32 chk = Abs(split->mousePos.x - finder.x);
				s32 a = chk;
				
				chk = Min(chk, Abs(split->mousePos.x - (finder.x + finder.w)));
				
				if (chk < 10) {
					if (chk == a)
						this->zoom.modifying = -1;
					else
						this->zoom.modifying = 1;
				} else {
					this->zoom.modifying = 8;
				}
			}
			
			switch (this->zoom.modifying) {
				case -1:
					this->zoom.start = ClampMax(mouseX, this->zoom.end - 0.0001);
					break;
				case 1:
					this->zoom.end = ClampMin(mouseX, this->zoom.start + 0.0001);
					break;
				case 8:
					if (this->zoom.start + xmod >= 0 && this->zoom.end + xmod <= 1.0) {
						this->zoom.start += xmod;
						this->zoom.end += xmod;
					}
					break;
			}
		} else
			this->zoom.modifying = 0;
	} else {
		Theme_SmoothStepToCol(&this->visual.findCol, Theme_GetColor(THEME_HIGHLIGHT, 25, 1.0), 0.25, 1.0, 0.0);
	}
	
	if (GeoGrid_Cursor_InRect(split, &waverect) && block == false) {
		f32 relPosX;
		f32 zoomRatio = this->zoom.end - this->zoom.start;
		f32 mod = (mouseX - this->mousePrevX) * zoomRatio;
		f32 scrollY = winCtx->input.mouse.scrollY;
		
		relPosX = this->zoom.start + ((f32)(split->mousePos.x - waverect.x) / waverect.w) * zoomRatio;
		
		if (winCtx->input.mouse.clickMid.hold) {
			if (this->zoom.start - mod >= 0 && this->zoom.end - mod <= 1.0) {
				this->zoom.start -= mod;
				this->zoom.end -= mod;
			}
		}
		
		if (scrollY) {
			if (scrollY > 0) {
				if (zoomRatio > 0.0001 * 0.5) {
					Math_SmoothStepToF(&this->zoom.start, relPosX, 0.15, zoomRatio, 0.0);
					Math_SmoothStepToF(&this->zoom.end, relPosX, 0.15, zoomRatio, 0.0);
				}
			} else {
				this->zoom.start += (0.0 - this->zoom.start) * zoomRatio;
				this->zoom.end += (1.0 - this->zoom.end) * zoomRatio;
			}
		}
		
		zoomRatio = this->zoom.end - this->zoom.start;
		
		if (zoomRatio < 0.0001) {
			this->zoom.start -= (0.0001 - zoomRatio) * 0.5;
			this->zoom.end += (0.0001 - zoomRatio) * 0.5;
		}
	}
	
	this->zoom.start = Clamp(this->zoom.start, 0.0, 1.0 - 0.0001);
	this->zoom.end = Clamp(this->zoom.end, 0.0001, 1.0);
	
	Math_SmoothStepToF(&this->zoom.vStart, this->zoom.start, 0.25, 1.0, 0.0);
	Math_SmoothStepToF(&this->zoom.vEnd, this->zoom.end, 0.25, 1.0, 0.0);
	
	f32 zoomRatio = this->zoom.vEnd - this->zoom.vStart;
	
	this->visual.playPos = GetPos((f32)sample->playFrame, &waverect, sample, this);
	this->visual.selectStart = GetPos((f32)sample->selectStart, &waverect, sample, this);
	this->visual.selectEnd = GetPos((f32)sample->selectEnd, &waverect, sample, this);
	
	if (sample->instrument.loop.count) {
		this->visual.loopA = GetPos((f32)sample->instrument.loop.start, &waverect, sample, this);
		this->visual.loopB = GetPos((f32)sample->instrument.loop.end, &waverect, sample, this);
	}
	
	Sampler_Draw_Grid(vg, &waverect, sample, this);
	Sampler_Draw_Finder(vg, &waverect, &finder, sample, this);
	Sampler_Draw_Selection(vg, &waverect, &finder, sample, this);
	if (sample->samplesNum * zoomRatio > waverect.w * 4)
		Sampler_Draw_Waveform_Block(vg, &waverect, sample, this);
	else
		Sampler_Draw_Waveform_Line(vg, &waverect, sample, this);
	Sampler_Draw_Position(vg, &waverect, &finder, sample, this);
	Sampler_Draw_LoopMarker(vg, &waverect, sample, this);
	
	// Adjust play position line by clicking
	
	if (Split_Cursor(split, 1) && GeoGrid_Cursor_InRect(split, &waverect) && block == false) {
		MouseInput* mouse = &winCtx->input.mouse;
		static s32 noPlayPosSet;
		f32 curPosX = split->mousePos.x - waverect.x;
		f32 prsPosX = split->mousePressPos.x - waverect.x;
		curPosX = sample->samplesNum * this->zoom.start + (f32)sample->samplesNum * (this->zoom.end - this->zoom.start) * (curPosX / waverect.w);
		prsPosX = sample->samplesNum * this->zoom.start + (f32)sample->samplesNum * (this->zoom.end - this->zoom.start) * (prsPosX / waverect.w);
		
		if (mouse->clickL.release && noPlayPosSet == false)
			sample->playFrame = curPosX;
		noPlayPosSet = false;
		
		if (mouse->doubleClick)
			sample->selectEnd = sample->selectStart = 0;
		
		if (mouse->clickL.hold == 0) {
			this->state.selecting = 0;
			this->state.selModify = 0;
		}
		
		if (mouse->clickL.hold && (Input_GetPressPosDist() > 4 || this->state.selecting || this->state.selecting)) {
			if (this->state.selModify == false && this->state.selecting == false) {
				if (sample->selectStart != sample->selectEnd) {
					if (Abs(sample->selectStart - prsPosX) > Abs(sample->selectEnd - prsPosX))
						this->state.lockPos = sample->selectStart;
					else
						this->state.lockPos = sample->selectEnd;
					this->state.selModify = 1;
				} else {
					sample->selectStart = sample->playFrame;
					this->state.selecting = 1;
				}
			}
			
			if (this->state.selecting) {
				sample->selectStart = Min(prsPosX, curPosX);
				sample->selectEnd = Max(prsPosX, curPosX);
			}
			
			if (this->state.selModify) {
				sample->selectStart = Min(this->state.lockPos, curPosX);
				sample->selectEnd = Max(this->state.lockPos, curPosX);
			}
			
			noPlayPosSet = true;
		}
	}
	
	this->mousePrevX = mouseX;
}