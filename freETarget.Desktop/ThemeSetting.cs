using Avalonia;
using Avalonia.Controls;
using Avalonia.Styling;
using freETarget.Properties;

namespace freETarget.Desktop {

    /** light or dark appearance of the application */
    static class ThemeSetting {

        public const string System = "System";
        public const string Light = "Light";
        public const string Dark = "Dark";

        /** applies the appearance of the settings: one of the two, or whatever the operating system uses */
        public static void apply() {
            if (Application.Current == null) {
                return;
            }
            switch (Settings.Default.theme) {
                case Light:
                    Application.Current.RequestedThemeVariant = ThemeVariant.Light;
                    break;
                case Dark:
                    Application.Current.RequestedThemeVariant = ThemeVariant.Dark;
                    break;
                default:
                    Application.Current.RequestedThemeVariant = ThemeVariant.Default;
                    break;
            }
        }

        /** gives a property of a control one of the colors from App.axaml, which changes with the appearance */
        public static T withBrush<T>(this T control, AvaloniaProperty property, string resource) where T : Control {
            control.Bind(property, control.GetResourceObservable(resource));
            return control;
        }
    }
}
