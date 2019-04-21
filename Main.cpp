# include <Siv3D.hpp> // Siv3D August 2016 v2

struct Leaf
{
	size_t leafID;
	Vec3 ypr;
	Vec3 basePos = RandomVec3(0.4);
	double rx = Random(-0.2, 0.2);
	double ry = Random(0.5);
	double startDelay = (0.4 + basePos.y)/0.8 * 0.12;
	double targetY = Random(0.5, 7.0);
	double targetR = Max(Random(0.1, 1.0) + (3.5 - Abs(targetY - 4.0)) * Random(0.2, 0.4), 0.2);
	double phiOffset = Random(360_deg);
	double lifeTime = Random(4.5, 5.0);
};

Image LoadShilouet(const FilePath& path)
{
	Image image(path);

	for (auto& pixel : image)
	{
		pixel.r = pixel.g = pixel.b = 255;
	}

	return image;
}

struct Note
{
	uint32 ch;
	uint32 noteNumber;
	int32 startMillisec;
	int32 lengthMillisec;

	double alpha;
	bool barPassed;
	bool onBar;
};

void Main()
{
	// http://oykenkyu.blogspot.com/2018/02/midi.html からダウンロード
	Midi::Open(L"INDETERMINATE_UNIVERSE_full_Ver1.1.mid");
	const auto score = Midi::GetScore();
	Array<Note> noteRects;
	uint32 minPitch = 127, maxPitch = 0;

	for (auto ch : step(static_cast<uint32>(score.size())))
	{
		for (const auto& note : score[ch])
		{
			noteRects.push_back({ ch, note.noteNumber, note.startMillisec, note.lengthMillisec, 1.0, false, false });
			minPitch = Min(minPitch, note.noteNumber);
			maxPitch = Max(maxPitch, note.noteNumber);
		}
	}

	const Font font(14, Typeface::Medium);
	Graphics::SetBackground(Color(0));
	Window::Resize(720, 720);
	Graphics3D::SetAmbientLightForward(ColorF(1.0));
	Graphics3D::SetLightForward(0, Light::None());
	Graphics3D::SetDepthStateForward(DepthState::TestOnly);
	Graphics3D::SetRasterizerStateForward(RasterizerState::SolidCullNone);
	const Texture textureSiv3D(LoadShilouet(L"Example/Siv3D-kun.png"), TextureDesc::Mipped);

	//const ColorF leafColor(0.81, 0.0, 0.46);
	const ColorF leafColor(0.2, 0.72, 0.46);

	const std::array<Texture, 6> textures =
	{
		Texture(Palette::White, L"leaf/1.png", TextureDesc::MippedSRGB),
		Texture(Palette::White, L"leaf/2.png", TextureDesc::MippedSRGB),
		Texture(Palette::White, L"leaf/3.png", TextureDesc::MippedSRGB),
		Texture(Palette::White, L"leaf/4.png", TextureDesc::MippedSRGB),
		Texture(Palette::White, L"leaf/5.png", TextureDesc::MippedSRGB),
		Texture(Palette::White, L"leaf/6.png", TextureDesc::MippedSRGB),
	};

	Array<Leaf> leaves;
	for (size_t i = 0; i < 120; ++i)
	{
		Leaf leaf;
		leaf.leafID = Random(textures.size() - 1);
		leaf.ypr = RandomVec3() * 360_deg;	
		leaves.push_back(leaf);
	}

	Stopwatch bg0;
	Stopwatch stopwatch;
	double volume = 1.0;

	while (System::Update())
	{
		if (Input::MouseL.clicked)
		{
			Midi::Play();
		}

		{
			const double scale = 0.15;
			const int32 offset = 80;
			const int32 offsetMillisec = static_cast<int32>(offset / scale);
			const RectF line(offset - 1, 0, 6, Window::Height());
			const double blockHeight = static_cast<double>(Window::Height()) / (maxPitch - minPitch + 1);

			if (!Midi::IsPlaying())
			{
				for (auto& note : noteRects)
				{
					note.alpha = 1.0;
					note.barPassed = false;
					note.onBar = false;
				}
			}

			const double pos = (Midi::GetPosSec() * 1000 - 100) * scale;
			const int32 bar = static_cast<int32>(Midi::GetPosSec() * 1000 - 100);
			const int32 left = bar - offsetMillisec;
			const int32 right = static_cast<int32>(left + Window::Width() / scale);

			Array<size_t> visibleNoteIndices;
			size_t index = 0;

			for (auto& note : noteRects)
			{
				if (right < note.startMillisec || (note.startMillisec + note.lengthMillisec) < left)
				{
					++index;
					continue;
				}

				note.onBar = note.startMillisec <= bar && bar <= (note.startMillisec + note.lengthMillisec);
				note.barPassed = note.startMillisec <= bar;

				if (note.barPassed)
				{
					note.alpha *= note.onBar ? 0.98 : 0.85;
				}

				visibleNoteIndices.push_back(index++);
			}

			Graphics2D::SetBlendState(BlendState::Default);

			for (auto& i : visibleNoteIndices)
			{
				const auto& note = noteRects[i];

				if (!note.barPassed)
				{
					const RectF r(note.startMillisec * scale + offset - pos, (maxPitch - note.noteNumber) * blockHeight, note.lengthMillisec * scale, blockHeight);
					RoundRect(r, 4).draw(ColorF(0.2, 0.25, 0.3));
				}
			}

			Graphics2D::SetBlendState(BlendState::Additive);

			for (auto& i : visibleNoteIndices)
			{
				const auto& note = noteRects[i];

				if (note.barPassed)
				{
					const RectF r(note.startMillisec * scale + offset - pos, (maxPitch - note.noteNumber) * blockHeight, note.lengthMillisec * scale, blockHeight);
					RoundRect(r, 5).drawShadow({ 0, 0 }, 12 + note.alpha * 8, 2 + note.alpha * 8, HSV(30 + note.ch * 100, 0.5, 1).toColorF(note.alpha * 0.4));
				}
			}

			for (auto& i : visibleNoteIndices)
			{
				const auto& note = noteRects[i];

				if (note.barPassed)
				{
					const RectF r(note.startMillisec * scale + offset - pos, (maxPitch - note.noteNumber) * blockHeight, note.lengthMillisec * scale, blockHeight);
					RoundRect(r, 4).draw(HSV(30 + note.ch * 100, 1, 1).toColorF(note.alpha));
				}
			}

			line.draw(Alpha(20));
		}
		Graphics2D::SetBlendState(BlendState::Default);
		Rect(200, 0, 720-200, 720).draw({ColorF(0.0,0.0),ColorF(0.0, 2.0), ColorF(0.0, 2.0), ColorF(0.0,0.0)});

		if (volume != 1.0)
		{
			Window::ClientRect().draw(ColorF(0.0, 1.0 - volume));	
		}

		if (Midi::GetPosSec() > 27.0)
		{
			const double alpha = Saturate(Midi::GetPosSec() - 27.0);
			font(L"MIDI:\noykenkyu.blogspot.com/2018/02/midi.html").draw(20, 20, ColorF(1.0, alpha));
		}


		Sphere(Vec3(0, 100, 0), 0).drawForward();
		Graphics::Render2D();

		Graphics3D::FreeCamera();

		if (Midi::GetPosSec() > 8.6)
		{
			bg0.start();
		}

		//if (Input::KeySpace.clicked)
		//{
		//	stopwatch.reset();
		//}
		//else if (Input::KeySpace.released)
		//{
		//	stopwatch.start();
		//	bg0.start();
		//}

		if (Midi::GetPosSec() > 22.3)
		{
			stopwatch.start();
		}

		if (Midi::GetPosSec() > 24.8)
		{
			volume = Saturate((27.8 - Midi::GetPosSec()) / 3.0);
			Midi::SetVolume(volume);
		}

		const double time = stopwatch.ms() / 1000.0;

		if (time)
		{
			for (auto& leaf : leaves)
			{
				const double pauseTime = 0.3;
				const double t = Max(time - pauseTime - leaf.startDelay, 0.0);
				const double timeY = t * 0.4;
				const double timeR = t * 0.05;

				const double rt = Min(1.0 - ((pauseTime + leaf.startDelay) - time) / (pauseTime + leaf.startDelay), 1.0);

				const Vec3 basePos = leaf.basePos + Vec3(leaf.rx * rt, leaf.ry * rt, 0.0);

				const double targetR = leaf.targetR + timeR;
				const double targetY = leaf.targetY + timeY;
				const Vec3 targetPos = Cylindrical(targetR, leaf.phiOffset + t * 50_deg, targetY);

				const double e = Easing::EaseOut<Easing::Circ>(Min(t * 0.6, 1.0));
				const Vec3 pos = Math::Lerp(basePos, targetPos, e);

				double lifeTimeRemaining = leaf.lifeTime - t;
				double a = Min(lifeTimeRemaining, 1.0);

				Sphere(0.4)
					.movedBy(pos + Vec3(3.3, -2.2, -1))
					.rollPitchYaw(Quaternion::RollPitchYaw(leaf.ypr.x, leaf.ypr.y, leaf.ypr.z))
					.rollPitchYaw(Quaternion::RollPitchYaw(0, t * 44_deg, t * 121_deg))
					.drawForward(textures[leaf.leafID], ColorF(leafColor, a));
			}
		}

		if (bg0.isActive())
		{
			const double x0Start = 180;
			const double x0Target = 540;

			const double x1Start = x0Start;
			const double x1Target = x0Target + 18;

			const double bg0t = Min(bg0.ms() / 800.0, 1.0);
			const double bg0e = Easing::EaseOut<Easing::Expo>(bg0t);

			const double bg1t = Min(bg0.ms() / 500.0, 1.0);
			const double bg1e = Easing::EaseOut<Easing::Quint>(bg1t);

			const double bg2t = Min(bg0.ms() / 6000.0, 1.0);
			const double bg2e = 1.0 - Easing::EaseOut<Easing::Linear>(bg2t);

			const double thickness = time ? Max(0.0, 5.0 - time * 6.0) : 17.0 * bg2e + 5.0;

			RectF(Math::Lerp(x0Start + thickness, x0Target, bg0e), 0, thickness, Window::Height())
				.draw(ColorF(leafColor, thickness / 5.0));

			if (bg2e)
			{
				Graphics2D::SetBlendState(BlendState::Additive);
				RectF(Math::Lerp(x0Start + thickness, x0Target, bg0e), 0, thickness, Window::Height())
					.stretched(-2.0, 0.0)
					.draw(ColorF(0.8, 0.3, 0.6, bg2e));
				Graphics2D::SetBlendState(BlendState::Default);
			}

			if (time == 0.0)
			{
				textureSiv3D.scale(0.64).drawAt(Math::Lerp(x1Start, x1Target, bg1e), 574, leafColor);
			}

			if (bg2e)
			{
				Graphics2D::SetBlendState(BlendState::Additive);
				textureSiv3D.scale(0.63).drawAt(Math::Lerp(x1Start, x1Target, bg1e), 574, ColorF(0.8, 0.3, 0.6, bg2e));
				Graphics2D::SetBlendState(BlendState::Default);
			}
		}
	}
}
