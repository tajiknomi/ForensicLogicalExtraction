using System;
using System.Windows;
using Wrapper;

namespace ForensicExtractorGUI
{
    public partial class MainWindow : Window
    {
        private ForensicExtractorWrapper extractor;

        public MainWindow()
        {
            InitializeComponent();
            // Initialize the wrapper with adb path (adjust if necessary)
            extractor = new ForensicExtractorWrapper("adb");
            LoadDevices();
        }

        private void LoadDevices()
        {
            DevicesComboBox.Items.Clear();

            int numDevices = extractor.NumOfConnectedAdbDevices();
            if (numDevices == 0)
            {
                DevicesComboBox.Items.Add("No devices found");
            }
            else
            {
                // For demonstration; you may want to populate real device names
                extractor.ShowConnectedDevices();
                DevicesComboBox.Items.Add("Device 1");
                DevicesComboBox.Items.Add("Device 2");
            }

            DevicesComboBox.SelectedIndex = 0;
        }

        private void RefreshBtn_Click(object sender, RoutedEventArgs e)
        {
            LoadDevices();
            AppendOutput("Device list refreshed.");
        }

        private void ProceedBtn_Click(object sender, RoutedEventArgs e)
        {
            OutputTextBox.Clear();
            StatusText.Text = "Running selected operations...";

            ProceedBtn.IsEnabled = false;

            if (ChkShowDevices.IsChecked == true)
            {
                AppendOutput("Running: Show Connected Devices");
                extractor.ShowConnectedDevices();
            }

            if (ChkExtractDeviceInfo.IsChecked == true)
            {
                AppendOutput("Running: Extract Device Info");
                int result = extractor.ExtractDeviceInfo("device_info.json");
                AppendOutput($"ExtractDeviceInfo returned: {result}");
            }

            if (ChkExtractApps.IsChecked == true)
            {
                AppendOutput("Running: Extract Installed Apps List");
                int result = extractor.ExtractUserInstalledAppsList("UserInstalledAppsList.json");
                AppendOutput($"ExtractUserInstalledAppsList returned: {result}");
            }

            if (ChkExtractSMS.IsChecked == true)
            {
                AppendOutput("Running: Extract SMS");
                int result = extractor.ExtractSMS("sms.txt");
                AppendOutput($"ExtractSMS returned: {result}");
            }

            if (ChkExtractCallLogs.IsChecked == true)
            {
                AppendOutput("Running: Extract Call Logs");
                int result = extractor.ExtractCallLogs("calllogs.txt");
                AppendOutput($"ExtractCallLogs returned: {result}");
            }

            if (ChkPullMedia.IsChecked == true)
            {
                AppendOutput("Running: Pull Media");
                int result = extractor.PullMedia("media");
                AppendOutput($"PullMedia returned: {result}");
            }

            if (ChkExtractMediaStore.IsChecked == true)
            {
                AppendOutput("Running: Extract MediaStore DB");
                int result = extractor.ExtractMediaStoreDb("mediastore.json");
                AppendOutput($"ExtractMediaStoreDb returned: {result}");
            }

            if (ChkExtractCalendar.IsChecked == true)
            {
                AppendOutput("Running: Extract Calendar Entities");
                int result = extractor.ExtractCalendarEntities("calendarEntities.json");
                AppendOutput($"ExtractCalendarEntities returned: {result}");
            }

            AppendOutput("All selected operations completed.");
            StatusText.Text = "Idle";
            ProceedBtn.IsEnabled = true;
        }

        private void AppendOutput(string text)
        {
            OutputTextBox.AppendText(text + "\n");
            OutputTextBox.ScrollToEnd();
        }
    }
}