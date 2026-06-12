(
  ;; Scene 1
  (scene-1
    (image "assets/backgrounds/first.bmp")
    (music "assets/sounds/myst.wav")
    (click-boxes
      (
        ;;(bounds 0.1 0.1 0.7 0.7)
        (bounds 128 72 100 100)
        (scene "scene-2")
        (sound "assets/sounds/click.wav")
      )
    )
    (sprites ())
  )

  ;; Scene 2
  (scene-2
    (image "assets/backgrounds/03.bmp")
    (music "assets/sounds/myst.wav")
    (click-boxes
      (
        (bounds 128 144 320 216)
        (scene "scene-7")
        (sound "assets/sounds/click.wav")
      )
      (
        (bounds 0 100 448 260)
        (scene "scene-8")
        (sound "assets/sounds/click.wav")
      )
    )
    (sprites ())
  )

  ;; Scene 3
  (scene-3
    (image "assets/backgrounds/03.bmp")
    (music "assets/sounds/myst.wav")
    (click-boxes
      (
        (bounds 192 144 56 88)
        (scene "scene-1")
        (sound "assets/sounds/click.wav")
      )
    )
    (sprites ())
  )

  ;; Scene 4
  (scene-4
    (image "assets/backgrounds/04.bmp")
    (music "assets/sounds/myst.wav")
    (click-boxes
      (
        (bounds 384 144 64 208)
        (scene "scene-4")
        (sound "assets/sounds/lags.wav")
      )
      (
        (bounds 0 0 80 20)
        (scene "scene-2")
        (sound "assets/sounds/click.wav")
      )
    )
    (sprites ())
  )

  ;; Scene 5
  (scene-5
    (image "assets/backgrounds/05.bmp")
    (music "assets/sounds/myst.wav")
    (click-boxes
      (
        (bounds 384 216 20 20)
        (scene "scene-4")
        (sound "assets/sounds/click.wav")
      )
      (
        (bounds 0 0 280 120)
        (scene "scene-8")
        (sound "assets/sounds/click.wav")
      )
    )
    (sprites ())
  )

  ;; Scene 6
  (scene-6
    (image "assets/backgrounds/03.bmp")
    (music "assets/sounds/myst.wav")
    (click-boxes
      (
        (bounds 192 144 300 333)
        (scene "scene-1")
        (sound "assets/sounds/click.wav")
      )
    )
    (sprites ())
  )

  ;; Scene 7
  (scene-7
    (image "assets/backgrounds/07.bmp")
    (music "assets/sounds/myst.wav")
    (click-boxes
      (
        (bounds 460 260 100 200)
        (scene "scene-7")
        (sound "assets/sounds/fillin.wav")
      )
      (
        (bounds 0 0 32 87)
        (scene "scene-2")
        (sound "assets/sounds/click.wav")
      )
    )
    (sprites ())
  )

  ;; Scene 8
  (scene-8
    (image "assets/backgrounds/08.bmp")
    (music "assets/sounds/myst.wav")
    (click-boxes
      (
        (bounds 384 216 20 40)
        (scene "scene-8")
        (sound "assets/sounds/spain.wav")
      )
      (
        (bounds 0 0 45 98)
        (scene "scene-5")
        (sound "assets/sounds/click.wav")
      )
    )
    (sprites 
      (
        (texture "assets/sprites/character.bmp")
        (position 0.5 0.5)
      )
    )
  )
)
