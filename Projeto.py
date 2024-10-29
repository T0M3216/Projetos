import flet as ft

def main(page: ft.Page):
    page.window_min_width = 500
    page.horizontal_alignment = 'center'
    page.vertical_alignmet = 'center'
    page.theme_mode = 'dark'
    
    text_login = ft.Container(
        ft.Text(
            value='Login',
            size=40,
            weight='bold',
        ),
        margin=ft.margin.only(left=140)
    )
    
    login.input = ft.Container(
        ft.TextField(
            hint_text='Login',
            width=300,
            border_color='white'.
        ),
         margin=ft.margin.only(left=50)
    )
    
    senha.input = ft.Container(
        ft.TextField(
            hint_text='Password'
            password=True,
            can_reveal_password=True,
            width=300,
            border_color='white'
        ),
         margin=ft.margin.only(left=50)
    )
    
    btn = ft.Container(
        ft.ElevatedButton(
            hint_text='Enter'
            width=200,
            height=50,
            style=ft.ButtonStyle(
                bgcolor={
                    ft.MaterialState.HOVERED:
                    ft.colors.WHITE,
                    ft.MaterialState.DEFAULT:
                    ft.colors.BLUE_GREY_300
                },
                side={
                    ft.MaterialState.DEFAULT:
                    ft.BorderSide(
                        color='white',
                        width=2
                    )
                },
                color={
                    ft.MaterialState.DEFAULT:
                    ft.colors.WHITE, 
                    ft.MaterialState.HOVERED:
                    ft.colors.BLUE_GREY
                }
            )
        ),
        margin= ft.margin.only(left=100)
    )
    
    
    layout = ft.Container(
        width=400,
        height=400,
        border_radius=20,
        shadow=ft.BoxShadow(
            blur_radius=5,
            color='white54'
        )
        gradient=ft.LinearGradient(
            begin=ft.alignment.top_left,
            end=ft.alignment.bottom_right,
            colors=ft.colors[ft.colors.WHITE38,
                             ft.colors.BLUE_GREY,
                             ft.colors.BLUE_GREY_300],
            stops=[0, 0.5, 1]
        ),
        content=ft.Column(
            spacing=30,
            alignment='center',
            controls=[
                text_login,
                login_input,
                senha_input,
                btn
            ]
        )
    )
    page.add(layout)
    
ft.app(target=main)