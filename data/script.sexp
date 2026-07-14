(
  ;; Scene 1
  (scene-1
    (background "data/backgrounds/first.bmp")
    (music "data/musics/myst.wav")
    (hotspot "To scene 2!" (128 72 100 100)
      (
        (sound "data/sounds/click.wav")
        (video t () "data/videos/01.mpg")
        (target "scene-2")
      )
    )
    (hotspot "Click!" (440 40 100 100)
      (
        (sound "data/sounds/click.wav")
        (set unlock t)
      )
    )
  )

  ;; Scene 2
  (scene-2
    (background "data/backgrounds/03.bmp")
    (music "data/musics/myst2.wav")
    (hotspot "To scene 7!" (128 144 320 216)
      (
        (sound "data/sounds/click.wav")
        (target "scene-7")
      )
    )
    (hotspot "To scene 8!" (0 100 448 260)
      (
        (sound "data/sounds/click.wav")
        (target "scene-8")
      )
    )
  )

  ;; Scene 3
  (scene-3
    (background "data/backgrounds/03.bmp")
    (music "data/musics/myst.wav")
    (hotspot "To scene 1!" (192 144 56 88)
      (
        (sound "data/sounds/click.wav")
        (target "scene-1")
      )
    )
  )

  ;; Scene 4
  (scene-4
    (background "data/backgrounds/04.bmp")
    (music "data/musics/myst.wav")
    (hotspot "To scene 4!" (384 144 64 208)
      (
        (sound "data/sounds/lags.wav")
        ;;(target "scene-4")
      )
    )
    (hotspot "To scene 2!" (0 0 80 20)
      (
        (sound "data/sounds/click.wav")
        (target "scene-2")
      )
    )
  )

  ;; Scene 5
  (scene-5
    (background "data/backgrounds/05.bmp")
    (music "data/musics/myst.wav")
    (hotspot "To scene 4!" (384 216 20 20)
      (
        (sound "data/sounds/click.wav")
        (target "scene-4")
      )
    )
    (hotspot "To scene 8!" (0 0 280 120)
      (
        (sound "data/sounds/click.wav")
        (target "scene-8")
      )
    )
  )

  ;; Scene 6
  (scene-6
    (background "data/backgrounds/03.bmp")
    (music "data/musics/myst.wav")
    (hotspot "To scene 1!" (192 144 300 333)
      (
        (sound "data/sounds/click.wav")
        (target "scene-1")
      )
    )
  )

  ;; Scene 7
  (scene-7
    (background "data/backgrounds/07.bmp")
    (music "data/musics/myst.wav")
    (hotspot "To scene 7!" (460 260 100 200)
      (
        (sound "data/sounds/fillin.wav")
        ;;(target "scene-7")
      )
    )
    (hotspot "To scene 2!" (0 0 32 87)
      (
        (sound "data/sounds/click.wav")
        (target "scene-2")
      )
    )
  )

  ;; Scene 8
  (scene-8
    (background "data/backgrounds/08.bmp")
    (music "data/musics/myst.wav")
    (hotspot "To scene 8!" (384 216 20 40)
      (
        (sound "data/sounds/spain.wav")
        ;;(target "scene-8")
      )
    )
    (hotspot "To scene 5!" (0 0 45 98)
      (
        (sound "data/sounds/click.wav")
        (target "scene-5")
      )
    )
  )
)
