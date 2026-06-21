(
  ;; Scene 1
  (scene-1
    (image "data/backgrounds/first.bmp")
    (music "data/sounds/myst.wav")
    (click-boxes
      (
        ;;(bounds 0.1 0.1 0.7 0.7)
        (bounds 128 72 100 100)
        (scene "scene-2")
        (sound "data/sounds/click.wav")
        (video "data/clips/01.mpg")
      )
    )
    (sprites ())
  )

  ;; Scene 2
  (scene-2
    (image "data/backgrounds/03.bmp")
    (music "data/sounds/myst.wav")
    (click-boxes
      (
        (bounds 128 144 320 216)
        (scene "scene-7")
        (sound "data/sounds/click.wav")
      )
      (
        (bounds 0 100 448 260)
        (scene "scene-8")
        (sound "data/sounds/click.wav")
      )
    )
    (sprites ())
  )

  ;; Scene 3
  (scene-3
    (image "data/backgrounds/03.bmp")
    (music "data/sounds/myst.wav")
    (click-boxes
      (
        (bounds 192 144 56 88)
        (scene "scene-1")
        (sound "data/sounds/click.wav")
      )
    )
    (sprites ())
  )

  ;; Scene 4
  (scene-4
    (image "data/backgrounds/04.bmp")
    (music "data/sounds/myst.wav")
    (click-boxes
      (
        (bounds 384 144 64 208)
        (scene "scene-4")
        (sound "data/sounds/lags.wav")
      )
      (
        (bounds 0 0 80 20)
        (scene "scene-2")
        (sound "data/sounds/click.wav")
      )
    )
    (sprites ())
  )

  ;; Scene 5
  (scene-5
    (image "data/backgrounds/05.bmp")
    (music "data/sounds/myst.wav")
    (click-boxes
      (
        (bounds 384 216 20 20)
        (scene "scene-4")
        (sound "data/sounds/click.wav")
      )
      (
        (bounds 0 0 280 120)
        (scene "scene-8")
        (sound "data/sounds/click.wav")
      )
    )
    (sprites ())
  )

  ;; Scene 6
  (scene-6
    (image "data/backgrounds/03.bmp")
    (music "data/sounds/myst.wav")
    (click-boxes
      (
        (bounds 192 144 300 333)
        (scene "scene-1")
        (sound "data/sounds/click.wav")
      )
    )
    (sprites ())
  )

  ;; Scene 7
  (scene-7
    (image "data/backgrounds/07.bmp")
    (music "data/sounds/myst.wav")
    (click-boxes
      (
        (bounds 460 260 100 200)
        (scene "scene-7")
        (sound "data/sounds/fillin.wav")
      )
      (
        (bounds 0 0 32 87)
        (scene "scene-2")
        (sound "data/sounds/click.wav")
      )
    )
    (sprites ())
  )

  ;; Scene 8
  (scene-8
    (image "data/backgrounds/08.bmp")
    (music "data/sounds/myst.wav")
    (click-boxes
      (
        (bounds 384 216 20 40)
        (scene "scene-8")
        (sound "data/sounds/spain.wav")
      )
      (
        (bounds 0 0 45 98)
        (scene "scene-5")
        (sound "data/sounds/click.wav")
      )
    )
    (sprites 
      (
        (texture "data/sprites/character.bmp")
        (position 0.5 0.5)
      )
    )
  )
)
